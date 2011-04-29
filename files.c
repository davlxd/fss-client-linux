/*
 * maintain .fss file in monitored direcotory
 *
 * Copyright (c) 2010, 2011 lxd <edl.eppc@gmail.com>
 * 
 * This file is part of File Synchronization System(fss).
 *
 * fss is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, 
 * (at your option) any later version.
 *
 * fss is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with fss.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "diff.h"
#include "wrap-sha1.h"
#include "files.h"

extern int error;


/* rootpath do not end with '/' */
static char rootpath[MAX_PATH_LEN];

static long linenum_to_send;
static off_t size_to_send;

/* these 2 global via called by call back function write_in() */
static FILE *finfo_fss;
static FILE *temp_hash_fss;

// client
static FILE *diff_remote_index;
static FILE *diff_local_index;

static int get_thefile(const char*, char*);
static int create_fss_dir(const char*);
static int connect_path(const char*, char*);
static int disconnect_path(const char*, char*);
static int get_rela_path(const char*, char *);
static int get_crlf_line(const char*, long , char*, int);
static int get_line(const char*, long, char*, int);
static int write_in(const char*, const struct stat*, int, struct FTW*);
static int write_in2(const char*, const struct stat*, int, struct FTW*);
static int fn(const char*, const struct stat*, int, struct FTW*);

int set_rootpath(const char *root_path)
{
  size_t strncpy_len, str_len;
  struct stat statbuf;
  char fullpath[MAX_PATH_LEN];

  if (stat(root_path, &statbuf) < 0) {
    perror("Path cannot be stat()");
    return 1;
  }

  str_len = strlen(root_path);

  strncpy_len = root_path[str_len-1] == '/' ? str_len-1 : str_len;

  // copy stripped path to rootpath as global viraible
  if (!strncpy(rootpath, root_path, strncpy_len)) {
    fprintf(stderr,
	    "@set_rootpath(): failed to strncpy %s to rootpath: %s\n",
	    fullpath, strerror(errno));
    return 1;
  }
  rootpath[strlen(rootpath)] = 0;

  return 0;
}
  
int update_files()
{
  struct stat statbuf;
  char fullpath[MAX_PATH_LEN];
  char fullpath0[MAX_PATH_LEN]; //temp.hash.fss
  char fullpath1[MAX_PATH_LEN]; //hash.fss

  get_thefile(TEMP_HASH_FSS, fullpath0);
  get_thefile(HASH_FSS, fullpath1);
  
  if (!strncpy(fullpath, rootpath, strlen(rootpath))) {
    fprintf(stderr, "@update_files(): strncpy() failed: %s",
	    strerror(errno));
    return 1;
  }
  fullpath[strlen(rootpath)] = 0;

  get_thefile(FSS_DIR, fullpath);

  if (create_fss_dir(fullpath)) {
    fprintf(stderr,
	    "@update_files(): create_fss_dir(%s) failed\n", fullpath);
    return 1;
  }
  
  if (connect_path(FINFO_FSS, fullpath)) {
    fprintf(stderr, "@update_files(): connect_path() failed\n");
    return 1;
  }
  
  if (!(finfo_fss = fopen(fullpath, "w+"))) {
    fprintf(stderr, "@update_files(): fopen(%s) fails\n", fullpath);
    return 1;
  }

  disconnect_path(FINFO_FSS, fullpath);
  
  if (connect_path(TEMP_HASH_FSS, fullpath)) {
    fprintf(stderr, "@update_files(): connect_path() failed\n");
    return 1;
  }

  if (!(temp_hash_fss = fopen(fullpath, "w+"))) {
    fprintf(stderr, "@update_files(): fopen(%s) fails\n", fullpath);
    return 1;
  }
  disconnect_path(TEMP_HASH_FSS, fullpath);
  disconnect_path(FSS_DIR, fullpath);

  if (nftw(fullpath, write_in, 10, FTW_DEPTH) != 0) {
    perror("@update_files(): ftw() failed");
    return 1;
  }

  if (0 != fflush(finfo_fss)) {
    perror("@update_files(): fflush(finfo_fss) fails.");
    return 1;
  }
  
  if (0 != fclose(finfo_fss)) {
    perror("@update_files(): fclose(finfo_fss) fails.");
    return 1;
  }

  if (0 != fflush(temp_hash_fss)) {
    perror("@update_files(): fflush(hash_fss) fails.");
    return 1;
  }
  
  if (0 != fclose(temp_hash_fss)) {
    perror("@update_files(): fclose(hashx_fss) fails.");
    return 1;
  }
  
  if (stat(fullpath1, &statbuf) < 0) {
    if (errno == ENOENT) {
      if (rename(fullpath0, fullpath1) < 0) {
	perror("@update_files(): rename fullpath0 to fullpath1 failed");
	return 1;
      }
      return 0;

    } else {
      perror("@update_files(): stat fullpath1 failed");
      return 1;
    }
  }

  char digest_temp[41];
  char digest_local[41];

  if (sha1_digest_via_fname(fullpath0, digest_temp)) {
    fprintf(stderr, "@update_files(): sha1_digest_via_fname() failed\n");
    return 1;
  }
  if (sha1_digest_via_fname(fullpath1, digest_local)) {
    fprintf(stderr, "@update_files(): sha1_digest_via_fname() failed\n");
    return 1;
  }
  
  if (strncmp(digest_temp, digest_local, 40) == 0) {
    
    if (remove(fullpath0) < 0) {
      perror("@update_files(): remove fullpath0 failed");
      return 1;
    }
  } else {
    if (remove(fullpath1) < 0) {
      perror("@update_files(): remove fullpath1 failed");
      return 1;
    }
    if (rename(fullpath0, fullpath1) < 0) {
      perror("@update_files(): rename fullpath0 to fullpath1 failed");
      return 1;
    }
  }
  printf(">>>> hash.fss and finfo.fss updateed\n");
  
  return 0;
}


int generate_diffs()
{
  char fullpath0[MAX_PATH_LEN]; // remote.hash.fss
  char fullpath1[MAX_PATH_LEN]; // hash.fss
  char fullpath2[MAX_PATH_LEN]; // diff.remote.index
  char fullpath3[MAX_PATH_LEN]; // diff.local.index

  get_thefile(REMOTE_HASH_FSS, fullpath0);
  get_thefile(HASH_FSS, fullpath1);
  get_thefile(DIFF_REMOTE_INDEX, fullpath2);
  get_thefile(DIFF_LOCAL_INDEX, fullpath3);


  char digest_remote[41];
  char digest_local[41];

  if (sha1_digest_via_fname(fullpath0, digest_remote)) {
    fprintf(stderr, "@generate_diffs(): sha1_digest_via_fname() failed\n");
    return 1;
  }
  if (sha1_digest_via_fname(fullpath1, digest_local)) {
    fprintf(stderr, "@generate_diffs(): sha1_digest_via_fname() failed\n");
    return 1;
  }

  if (strncmp(digest_remote, digest_local, 40) == 0)
    return DIFF_IDENTICAL;
  
  else {
    if (diff(fullpath0, fullpath1, fullpath2, fullpath3, NULL)) {
      fprintf(stderr, "@generate_diffs(): diff() failed\n");
      return 1;
    }
  }

  /* the following code is added temporaly */
  struct stat statbuf;
  off_t size2, size3;
  
  if (stat(fullpath2, &statbuf) < 0) {
    fprintf(stderr, "@generate_diffs(): stat() failed\n");
    return 1;
  }
  size2 = statbuf.st_size;

  if (stat(fullpath3, &statbuf) < 0) {
    fprintf(stderr, "@generate_diffs(): stat() failed\n");
    return 1;
  }
  size3 = statbuf.st_size;

  if (size2 == 0 && size3 == 0)
    return DIFF_IDENTICAL;
  if (size2 == 0)
    return DIFF_LOCAL_UNIQ;
  if (size3 == 0)
    return DIFF_REMOTE_UNIQ;
  /* end */


  return 0;
}

static int connect_path(const char *path1, char *path0)
{
  char *ptr;
  //char *ptr1;
  ptr = path0 + strlen(path0);
  
  if (*(ptr-1) != '/') {
    *ptr++ = '/';
    *ptr = 0;
  }

  if (*path1 != '/') {
    if (!strncpy(ptr, path1, strlen(path1))) {
      fprintf(stderr,
	      "@connect_path(): fail to strncpy %s to %s: %s\n",
	      path1, path0, strerror(errno));
      return 1;
    }
    *(ptr+strlen(path1)) = 0;
    
  } else {
    //ptr1 = path1 + 1;  
    if (!strncpy(ptr, path1 + 1, strlen(path1) - 1)) {
      fprintf(stderr,
	      "@connect_path(): fail to strncpy %s to %s: %s\n",
	      path1 + 1, ptr, strerror(errno));
      return 1;
    }
    *(ptr+strlen(path1)-1) = 0;
  }

  return 0;
}


static int disconnect_path(const char *path1, char *path0)
{
  char *ptr;
  ptr = path0 + strlen(path0);
  *(ptr - strlen(path1) - 1) = 0;
  return 0;
}


static int write_in(const char *path, const struct stat *ptr,
		    int flag, struct FTW *fb)
{
  int rv;
  // esapce rootpath
  if (strncmp(path, rootpath, strlen(path)) == 0)
    return 0;

  //TODO: trick
  if (!INCLUDE_HIDDEN && strstr(path, "/."))
    return 0;
  if (strncmp(FSS_DIR, path+fb->base, strlen(FSS_DIR) == 0))
    return 0;
  
  char digest[41];


  if ((rv = sha1_digest_via_fname_fss(path, rootpath, digest)) == 1) {
    fprintf(stderr, "@write_in(): sha1_digest_via_fname_fss(%s) fails\n",
	    path);
    return 1;
  }

  // if sha1_digest_via_fname_fss() return 2,
  // means target file/dir dosen't exist, which happens
  // when user remove files continously, and nftw() is so fast that
  // catch a being deleting file

  if (rv == 0) {
  
    if (EOF == fputs(digest, temp_hash_fss)) {
      perror("@write_in(): fputs fails.");
      return 1;
    }
    if (EOF == fputc('\n', temp_hash_fss)) {
      perror("@write_in(): fputc() \\n fails.");
      return 1;
    }

  
    if (EOF == fputs(path, finfo_fss)) {
      perror("@write_in(): fputs fails.");
      return 1;
    }
    if (EOF == fputc('\n', finfo_fss)) {
      perror("@write_in(): fputc() \\n fails.");
      return 1;
    }
  }


  return 0;
}
  


static int write_in2(const char *path, const struct stat *ptr,
		    int flag, struct FTW *fb)
{
  // only include regular files and dirs
  if (!S_ISDIR(ptr->st_mode) && !S_ISREG(ptr->st_mode))
    return 0;
  // esapce rootpath
  if (strncmp(path, rootpath, strlen(path)) == 0)
    return 0;
  // TODO: trick
  // escapce hidden files
  if (!INCLUDE_HIDDEN && strstr(path, "/."))
    return 0;
  //escapce .fss
  if (strncmp(FSS_DIR, path+fb->base, strlen(FSS_DIR) == 0))
    return 0;

  int rv;
  int typeflag = 0; // flag for entry type, regfile->0, dir->1
  size_t str_len, len;
  char sha1[41];
  char hash[42];
  char relapath[MAX_PATH_LEN];
  char record[MAX_PATH_LEN];

  if (get_rela_path(path, relapath)) {
    fprintf(stderr, "@write_in(): get_rela_path() failed");
    return 1;
  }

  if (S_ISDIR(ptr->st_mode))
    typeflag = 1;
  
  if ((rv = compute_hash(path, rootpath, sha1, hash)) == 1) {
    fprintf(stderr, "@write_in(): compute_hash() failed");
    return 1;
  }


  // if sha1_digest_via_fname_fss() return ENOENT(2)
  // means target file/dir dosen't exist, which happens
  // when user remove files continously, and nftw() is so fast that
  // catch a being deleting file
  if (rv == ENOENT)
    return 0;

  strncat(record, hash, strlen(hash));
  strncat(record, "\n", strlen("\n"));

  if (EOF == (fputs(record, temp_hash_fss))) {
    perror("@write_in(): fputs() failed");
    return 1;
  }

  snprintf(record, MAX_PATH_LEN, "%d", typeflag);
  record[1] = 0;

  strncat(record, sha1, strlen(sha1));
  strncat(record, "\n", strlen("\n"));
  strncat(record, relapath, strlen(relapath));
  strncat(record, "\n", strlen("\n"));

  str_len = strlen(record);
  len = snprintf(record+str_len,
		 MAX_PATH_LEN-str_len,
		 "%ld\n%ld\r\n", ptr->st_size, ptr->st_mtime);
  record[str_len+len] = 0;

  if (EOF == (fputs(record, finfo_fss))) {
    perror("@write_in(): fputs() failed");
    return 1;
  }
 
  return 0;
}
  
  
static int create_fss_dir(const char *path)
{
  struct stat statbuf;

  if (lstat(path, &statbuf) < 0) {
    if (errno == ENOENT ) {
      if (mkdir(path, 0770) < 0) {
	fprintf(stderr,
		"@create_fss_dir(): fail to mkdir(%s): %s",
		path, strerror(errno));
	return 1;
      }
      return 0;
      
    } else {
      fprintf(stderr,
	      "@create_fss_dir(): fail to lstat(%s): %s",
	      path, strerror(errno));
      return 1;
    }
  }

  if (!S_ISDIR(statbuf.st_mode)) {
    if (remove(path) < 0) {
      fprintf(stderr,
	      "@create_fss_dir(): fail to remove(%s): %s",
	      path, strerror(errno));
      return 1;
    }
    if (mkdir(path, 0770) < 0) {
      fprintf(stderr,
	      "@create_fss_dir(): fail to mkdir(%s): %s",
	      path, strerror(errno));
      return 1;
    }
  }

  return 0;
}

// get one line lined by CRLF
static int get_crlf_line(const char *fname, long linenum,
			 char *buffer, int len)

{
  FILE *file;
  char *ptr;
  int c, d;
  int i;
  long num = 0;

  if (!(file = fopen(fname, "rb"))) {
    perror("@get_crlf_line(): fopen() failed");
    return 1;
  }

  num = 0;
  c = getc(file);
  while ((num < (linenum-1)) && (c != EOF)) {
    d = c;
    c = getc(file);
    if ((d == '\r') && (c == '\n'))
      num++;
  }
  if (c == EOF) {
    fprintf(stderr, "@get_crlf_line(): linemum %ld is largner %ld\n", 
	    linenum, num);
    return 1;
  }

  c = getc(file);
  ptr = buffer;
  i = 0;
  while ((c != EOF) && (c != '\r') && (i < len)) {
    *ptr++ = c;
    i++;
    c = getc(file);
  }
  *ptr = 0;

  if (c == EOF) {
    fprintf(stderr, "@get_crlf_line(): unexpcted EOF\n");
    return 1;
  }
  if (i >= len) {
    fprintf(stderr,
	    "@get_crlf_line(): line %s.., exceeds buffer length %d\n",
	    buffer, len);
    return 1;
  }

  return 0;   
}

static int get_line(const char *fname, long linenum,
		    char *buffer, int maxlen)
{
  FILE *file;
  int c;
  int num;
  
  if (!(file = fopen(fname, "rb"))) {
    fprintf(stderr, "@get_line(): fopen(%s) fails\n", fname);
    return 1;
  }
  
  num = 0;
  while((num < (linenum-1)) && (c = getc(file)) != EOF)
    if (c == '\n')
      num++;

  if (c == EOF) {
    fprintf(stderr, "@get_line(): linenum %ld is larger\n", linenum);
    return 1;
  }

  if (fgets(buffer, maxlen, file) == NULL) {
    perror("@get_line(): fgets() failed\n");
    return 1;
  }

  /* if..., actually it must be */
  if (buffer[strlen(buffer)-1] == '\n') {
    buffer[strlen(buffer)-1] = 0;
  }
    

  if (0 != fclose(file)) {
    perror("@get_line(): fclose(file) failed");
    return 1;
  }

  return 0;
}

/* assume rela_path is big enought*/
static int get_rela_path(const char *fullpath, char *rela_path)
{
  size_t len;
  /* minus 1 means omit '/' between rootpath and rela_path */
  len = strlen(fullpath) - strlen(rootpath) -1;
  
  if (strncpy(rela_path,
	      fullpath+strlen(rootpath)+1,
	      len) == NULL) {
    perror("@get_rela_path(): strncpy() failed");
    return 1;
  }
  rela_path[len]= 0;
  
  return 0;
}


/* protocol.h/c get hash.fss's st_mtime to compare with remote's */
time_t hash_fss_mtime()
{
  char fullpath[MAX_PATH_LEN];
  struct stat statbuf;

  get_thefile(HASH_FSS, fullpath);
  
  if (stat(fullpath, &statbuf) < 0) {
    perror("hash_fss_mtime(): stat() failed");
    return 1;
  }

  return statbuf.st_mtime;
}

int remove_diffs()
{
  char fullpath0[MAX_PATH_LEN];
  char fullpath1[MAX_PATH_LEN];
  
  get_thefile(DIFF_LOCAL_INDEX, fullpath1);
  get_thefile(DIFF_REMOTE_INDEX, fullpath0);

  errno = 0;
  if (remove(fullpath0) < 0 && errno != ENOENT) {
    perror("@remove_diffs(): remove fullpath0 failed");
    return 1;
  }

  errno = 0;
  if (remove(fullpath1) < 0 && errno != ENOENT) {
    perror("@remove_diffs(): remove fullpath1 failed");
    return 1;
  }

  return 0;
}

int remove_files()
{
  char fullpath0[MAX_PATH_LEN]; // diff.local.index
  char fullpath1[MAX_PATH_LEN]; // finfo.fss
  char buf[MAX_PATH_LEN];
  char record[MAX_PATH_LEN];
  long linenum_to_delete;
  struct stat statbuf;
  FILE *file;

  get_thefile(DIFF_LOCAL_INDEX, fullpath0);
  get_thefile(FINFO_FSS, fullpath1);

  if (stat(fullpath0, &statbuf) < 0) {
    perror("@remove_files(): stat failed");
    return 1;
  }

  if (statbuf.st_size == 0)
    return 0;

  if (!(file = fopen(fullpath0, "rb"))) {
    fprintf(stderr, "@remove_files(): fopen() failed\n");
    return 1;
  }

  while (fgets(buf, MAX_PATH_LEN, file) != NULL) {
    if ((linenum_to_delete = strtol(buf, NULL, 10)) == 0) {
      perror("@remove_files(): strtol() failed");
      return 1;
    }

    if (get_line(fullpath1, linenum_to_delete, record, MAX_PATH_LEN)) {
      fprintf(stderr, "@remove_files(): get_line() failed\n");
      return 1;
    }

    if (stat(record, &statbuf) < 0) {
      perror("@remove_files(): stat failed\n");
      return 1;
    }
    if (S_ISDIR(statbuf.st_mode)) {
      if (remove_dir(record) < 0) {
	perror("@remove_files(): remove_dir() failed\n");
	return 1;
      }
      continue;
    }

    if (remove(record) < 0 && errno != ENOENT) {
      perror("@remove_files(): remove() failed");
      return 1;
    }
  }
  if (ferror(file)) {
    perror("@remove_files(): fgets() failed");
    return 1;
  }
  return 0;

}



int send_del_index(int sockfd)
{
  char fullpath[MAX_PATH_LEN];
  get_thefile(DIFF_REMOTE_INDEX, fullpath);

  if (send_file(sockfd, fullpath)) {
    fprintf(stderr, "@send_del_index(): send_file() failed\n");
    return 1;
  }

  return 0;
}

int send_file_via_linenum(int sockfd)
{

  char fullpath[MAX_PATH_LEN];
  char record[MAX_PATH_LEN];

  get_thefile(FINFO_FSS, fullpath);

  memset(record, 0, MAX_PATH_LEN);

  if (get_line(fullpath, linenum_to_send, record, MAX_PATH_LEN)) {
    fprintf(stderr, "@send_file_via_linenum(): get_line() failed\n");
    return 1;
  }


  if (send_file(sockfd, record)) {
    fprintf(stderr, "@send_file_via_linenun(): send_file() failed\n");
    return 1;
  }

  return 0;
}

int send_file(int sockfd, const char *fname)
{
  printf(">>>> in send_file(): sending  %s, size=%ld    ...  ", fname, size_to_send);
  char buf[BUF_LEN];
  size_t len;
  off_t size;
  FILE *file;

  if (size_to_send == 0)
    return 0;

  if (!(file = fopen(fname, "rb"))) {
    fprintf(stderr, "@send_file(): fopen(%s) fails\n", fname);
    return 1;
  }

  size = 0;
  memset(buf, 0, BUF_LEN);
  while((size < size_to_send) &&
	(len = fread(buf, sizeof(char), BUF_LEN, file))) {
    size += len;
    if (write(sockfd, buf, len) < 0) {
      perror("@send_file(): write() fails");
      return 1;
    }
    memset(buf, 0, BUF_LEN);
  }

  if (0 != fclose(file)) {
    perror("@send_file(): fclose(file) fails.");
    return 1;
  }
  printf("  DONE\n"); 

  return 0;
}

int send_del_index_info(int sockfd, const char *prefix)
{
  char fullpath[MAX_PATH_LEN];
  
  get_thefile(DIFF_REMOTE_INDEX, fullpath);

  if (send_entryinfo(sockfd, fullpath, prefix, NULL) == 1) {
    fprintf(stderr,
	    "@send_del_index_size(): send_file_mtime_size() faield\n");
    return 1;
  }

  return 0;
}


int send_entryinfo_via_linenum(int sockfd, long linenum,
			       const char *prefix0, const char *prefix1)
{

  int rv;
  char fullpath[MAX_PATH_LEN];
  char record[MAX_PATH_LEN];

  get_thefile(FINFO_FSS, fullpath);

  memset(record, 0, MAX_PATH_LEN);
  if (get_line(fullpath, linenum, record, MAX_PATH_LEN)) {
    fprintf(stderr, "@send_entryinfo_via_linenum(): get_line() failed\n");
    return 1;
  }

  if ((rv = send_entryinfo(sockfd, record, prefix0, prefix1)) == 1) {
    fprintf(stderr, "@send_entryinfo_via_linenum(): "\
	    "send_entryinfo() failed\n");
    return 1;

  } else if (rv == PREFIX1_SENT)
    return PREFIX1_SENT;

  return PREFIX0_SENT;
}


int send_entryinfo_via_linenum2(int sockfd, long linenum,
				const char *prefix0, const char *prefix1)
{


}


int send_entryinfo2(int sockfd, const char *fname,
		    const char *prefix0, const char *prefix1)
{


  return 0;
}

int send_entryinfo(int sockfd, const char *fname,
		   const char *prefix0, const char *prefix1)
{

  char rela_fname[MAX_PATH_LEN];
  char msg[MAX_PATH_LEN];
  struct stat statbuf;
  int len, str_len;
  int rv;
  int temp_errno;

  if ((stat(fname, &statbuf) < 0) && (errno != ENOENT) ) {
    perror("@send_entryinfo(): stat failed");
    return 1;
  }
  temp_errno = errno;

  // if it is a dir, append prefix1 (DIR_INFO)
  if (S_ISDIR(statbuf.st_mode)) {
    if (strncpy(msg, prefix1, strlen(prefix1)) == NULL) {
      perror("@send_entryinfo(): strncpy() failed");
      return 1;
    }
    msg[strlen(prefix1)] = 0;  rv = PREFIX1_SENT;

    // else append prefix0 (FILE_INFO)
  } else {
    if (strncpy(msg, prefix0, strlen(prefix0)) == NULL) {
      perror("@send_entryinfo(): strncpy() failed");
      return 1;
    }
    msg[strlen(prefix0)] = 0;  rv = PREFIX0_SENT;
  }
  
  if (get_rela_path(fname, rela_fname)) {
    fprintf(stderr, "@send_entryinfo(): get_rela_path() failed\n");
    return 1;
  }

  str_len = strlen(msg);
  if (!strncpy(msg+str_len, rela_fname, strlen(rela_fname))) {
    fprintf(stderr, "@send_file_via_fname(): strncpy() failed");
    return 1;
  }
  msg[str_len+strlen(rela_fname)] = 0;
  str_len = strlen(msg);

  // if this file or dir has been deleted while syncing
  if (temp_errno == ENOENT) {
    if((len = snprintf(msg+str_len,
		       MAX_PATH_LEN - str_len,
		       "\n%ld\n%ld", (long)1, (long)0)) < 0) {
      perror("@send_entryinfo(): snprintf() failed");
      return 1;
    }
    size_to_send = 0;

  } else {
    if ((len = snprintf(msg + str_len,
			MAX_PATH_LEN - str_len,
			"\n%ld", statbuf.st_mtime)) < 0) {
      perror("@send_entryinfo(): snprintf() failed");
      return 1;
    }
    msg[str_len + len] = 0;

    str_len = strlen(msg);
    if ((len = snprintf(msg + str_len,
			MAX_PATH_LEN - str_len,
			"\n%ld", statbuf.st_size)) < 0) {
      perror("@send_entryinfo(): snprintf() failed");
      return 1;
    }
    msg[str_len + len] = 0;
    size_to_send = statbuf.st_size;
  }

  if (send_msg(sockfd, msg)) {
    fprintf(stderr, "@send_entryinfo(): send_msg() failed\n");
    return 1;
  }

  printf(">>>> just send --%s--\n", msg);
  return rv;
}

int send_msg(int sockfd, const char *msg)
{
  if(write(sockfd, msg, strlen(msg)) < 0) {
    perror("@send_msg(): write() failed");
    return 1;
  }

  return 0;
}



int send_linenum_or_done(int sockfd, int if_init,
			 const char *prefix0, const char *prefix1)
{
  char fullpath[MAX_PATH_LEN];
  char buf[MAX_PATH_LEN];

  if (!strncpy(buf, prefix0, strlen(prefix0))) {
    perror("@send_linenum_or_done(): strncpy failed");
    return 1;
  }
  buf[strlen(prefix0)] = 0;

  get_thefile(DIFF_REMOTE_INDEX, fullpath);
  
  if (if_init) {
    if ((diff_remote_index = fopen(fullpath, "rb")) == NULL) {
      perror("@send_linenum_or_done(): fopen() failed");
      return 1;
    }
  }

  if (fgets(buf+strlen(prefix0), MAX_PATH_LEN-strlen(prefix0),
	    diff_remote_index) != NULL) {
    if (send_msg(sockfd, buf)) {
      fprintf(stderr, "@send_linenum_or_done(): send_msg failed\n");
      return 1;
    }
    
  } else if (feof(diff_remote_index)) {
    if (send_msg(sockfd, prefix1)) {
      fprintf(stderr, "@send_linenum_or_done(): send_msg failed\n");
      return 1;
    }
    if (fclose(diff_remote_index) == EOF) {
      perror("@send_linenum_or_done(): fclose() failed\n");
      return 1;
    }
    return PREFIX1_SENT;

  } else {
    fprintf(stderr, "@send_liennum_or_done(): fgets() failed\n");
    return 1;
  }
  
  return PREFIX0_SENT;

}


int send_entryinfo_or_reqhashinfo(int sockfd, int ifinit,
				  const char *prefix0,
				  const char *prefix1, const char *prefix2)
{
  int rv;
  char fullpath[MAX_PATH_LEN];
  char buf[MAX_PATH_LEN];

  get_thefile(DIFF_LOCAL_INDEX, fullpath);
  
  if (ifinit) {
    if ((diff_local_index = fopen(fullpath, "rb")) == NULL) {
      perror("@send_entryinfo_or_done(): fopen() failed");
      return 1;
    }
  }

  if (fgets(buf, MAX_PATH_LEN, diff_local_index) != NULL) {
    
    if ((linenum_to_send = strtol(buf, NULL, 10)) == 0) {
      perror("@send_entryinfo_or_done(): strtol() failed");
      return 1;
    }

    if ((rv = send_entryinfo_via_linenum(sockfd, linenum_to_send,
					 prefix0, prefix1)) == 1) {
      fprintf(stderr, "@send_entryinfo_or_done(): " \
	      "send_entryinfo_via_linenum() failed\n");
      return 1;
    }

    if (rv == PREFIX1_SENT)
      return PREFIX1_SENT;

    else if (rv == PREFIX0_SENT)
      return PREFIX0_SENT;

  } else if (feof(diff_local_index)) {
    if (send_msg(sockfd, prefix2)) {
      fprintf(stderr, "@send_entryinfo_or_done(): send_msg() failed\n");
      return 1;
    }
    if (fclose(diff_local_index) == EOF) {
      perror("@send_entryinfo_or_done(): fclose() failed");
      return 1;
    }
    linenum_to_send = -1;
    return PREFIX2_SENT;

  } else {
    fprintf(stderr, "@send_entryinfo_or_done(): fgets() failed\n");
    return 1;
  }
  
  return PREFIX0_SENT;
}



int receive_hash_fss(int sockfd, off_t sz)
{
  char fullpath[MAX_PATH_LEN];

  get_thefile(HASH_FSS, fullpath);
  
  if (receive_file(sockfd, fullpath, sz)) {
    fprintf(stderr, "@receive_hash_file(): receive_file() fail\n");
    return 1;
  }
  return 0;
}

int receive_common_file(int sockfd, const char *rela_fname, off_t sz)
{
  char fullpath[MAX_PATH_LEN];
  char relaname[MAX_PATH_LEN];
  struct stat statbuf;
  char *token, *token1;
  mode_t default_mode;

  if (!strncpy(fullpath, rootpath, strlen(rootpath))) {
    perror("@receive_common_file(): strncpy rootpath to fullpath failed");
    return 1;
  }
  fullpath[strlen(rootpath)] = 0;

  /* set default mode_t */
  if (stat(fullpath, &statbuf) < 0) {
    perror("@receive_common_file(): stat failed");
    return 1;
  }
  default_mode = statbuf.st_mode;

  if (!strncpy(relaname, rela_fname, strlen(rela_fname))) {
    perror("@receive_common_file(): strncpy rela_fname to relaname failed");
    return 1;
  }
  relaname[strlen(rela_fname)] = 0;

  /* the following code mkdir if specific dir dosen't exist
   * token1 is next token of token,
   * if token1 is NULL, so token should be a file name,
   * then folowing  if-dir-exsist-judge-algorithm
   * should not include this particular token, so escape while() */
  token = strtok(relaname, "/");
  token1 = strtok(NULL, "/");
  while(token && token1) {
    if (connect_path(token, fullpath)) {
      fprintf(stderr, "@receive_common_file(): connect_path() failed\n");
      return 1;
    }
    if (stat(fullpath, &statbuf) < 0) {
      if (errno == ENOENT) {
	if (mkdir(fullpath, default_mode) < 0) {
	  perror("@receive_common_file(): mkdir() failed");
	  return 1;
	}
      } else {
	perror("@receive_common_file(): stat() failed\n");
	return 1;
      }
    }
    token = token1;
    token1 = strtok(NULL, "/");
  }
    
  if (connect_path(token, fullpath)) {
    fprintf(stderr, "@receive_common_file(): connect_path failed\n");
    return 1;
  }

  if (receive_file(sockfd, fullpath, sz)) {
    fprintf(stderr, "@recive_common_file(): receive_file() failed\n");
    return 1;
  }

  return 0;
}

int receive_file(int sockfd, const char *fname, off_t sz)
{
  printf(">>>> in receive_file(), receiving: %s, size=%ld\n", fname, sz); fflush(stdout);

  char buf[BUF_LEN];
  ssize_t len;
  off_t size;
  FILE *file;

  if (!(file = fopen(fname, "w+"))) {
    fprintf(stderr, "@receive_file(): fopen(%s) fails\n", fname);
    return 1;
  }

  /* touch it if it is an empty file */
  if (sz == 0 ) {
    if (0 != fclose(file)) {
      perror("@receive_file(): fclose() failed");
      return 1;
    }
    return 0;
  }

  size = 0;
  memset(buf, 0, BUF_LEN);
  while((size < sz) && ((len = read(sockfd, buf, BUF_LEN)) > 0)) {
    size += len;
    if (fwrite(buf, sizeof(char), len, file) < len) {
      perror("@receive_file(): fwrite fails.");
      return 1;
    }
    memset(buf, 0, BUF_LEN);
  }
  
  if (len < 0) {
    perror("@receive_file(): read from socket fails");
    return 1;
  }
  
  if (0 != fflush(file)) {
    perror("@receive_file(): fflush(remote_hash_file) fails.");
    return 1;
  }
  
  if (0 != fclose(file)) {
    perror("@receive_file(): fclose(remote_hash_file) fails.");
    return 1;
  }
  printf(" received\n"); fflush(stdout);

  return 0;
}



int create_dir(const char *relafname)
{
  char fullpath[MAX_PATH_LEN];
  struct stat statbuf;
  mode_t mode;

  if (stat(rootpath, &statbuf) < 0) {
    perror("@create_dir(): stat() failed\n");
    return 1;
  }
  mode = statbuf.st_mode;

  if (!strncpy(fullpath, rootpath, strlen(rootpath))) {
    fprintf(stderr,
	    "@create_dir(): strncpy rootpath:%s to fpath:%s failed: %s\n",
	    rootpath, fullpath, strerror(errno));
    return 1;
  }
  fullpath[strlen(rootpath)] = 0;

  if (connect_path(relafname, fullpath)) {
    fprintf(stderr, "@create_dir(): connect_path failed\n");
    return 1;
  }

  errno = 0;
  if (stat(fullpath, &statbuf) < 0 ) {
    if (errno == ENOENT) {
      if (mkdir(fullpath, mode) < 0) {
	perror("@create_dir(): mkdir() failed\n");
	return 1;
      }
    } else {
      perror("@create_dir(): stat() failed\n");
      return 1;
    }
  }
  

  return 0;
}

// remove dir even contain stuff
int remove_dir(const char *fname)
{
  //FTW_DEPTH is important here, refer manual
  if (nftw(fname, fn, 10, FTW_DEPTH) != 0) {
    perror("@remove_dir(): nftw() failed");
    return 1;
  }

  return 0;
}

static int fn(const char *fname, const struct stat *sb,
	      int typeflag, struct FTW *ftwbuf)
{
  if (remove(fname) < 0) {
    perror("@fn(): remove() failed");
    return 1;
  }

  return 0;
}



static int get_thefile(const char *name, char* fpath)
{

  if (!strncpy(fpath, rootpath, strlen(rootpath))) {
    perror("@get_thefile(): strncpy() failed");
    return 1;
  }
  fpath[strlen(rootpath)] = 0;

  if (connect_path(FSS_DIR, fpath)) {
    fprintf(stderr, "@get_thefile(): connect_path() failed\n");
    return 1;
  }
  
  if (0 == strncmp(name, FSS_DIR, strlen(name)))
    return 0;

  if (connect_path(name, fpath)) {
    fprintf(stderr, "@get_thefile(): connect_path() failed\n");
    return 1;
  }

  return 0;

}
