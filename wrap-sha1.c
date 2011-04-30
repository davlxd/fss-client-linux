/*
 * wrap
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

#include "sha1.h"
#include "wrap-sha1.h"

extern int errno;

static int export_to_str(SHA1Context *sha1, char *digest_str);

int sha1_digest_via_fname(const char *fname, char *digest)
{
  FILE *file;
  if (!(file = fopen(fname, "rb"))) {
    fprintf(stderr,
	    "@sha1_digest_via_fname(): open %s fails: %s\n",
	    fname, strerror(errno));
    return 1;
  }
  if (sha1_file(file, digest)) {
    fprintf(stderr, "@sha1_digest_via_fname(): sha1_file() fails\n");
    return 1;
  }
  if (0 != fclose(file)) {
    perror("@sha1_digest_via_fname(): fclose() fails");
    return 1;
  }
  return 0;
}

int sha1_file(FILE *file, char *digest)
{
  SHA1Context sha;
  int len, i;
  unsigned char buf[SHA1_BUF_LEN];
  
  SHA1Reset(&sha);

  len = fread(buf, sizeof(unsigned char), SHA1_BUF_LEN, file);
  while(len) {
    SHA1Input(&sha, buf, len);
    len = fread(buf, sizeof(unsigned char), SHA1_BUF_LEN, file);
  }

  if (!SHA1Result(&sha)) {
    fprintf(stderr,
	    "@sha1_file(): SHA1Result() fails.\n");
    return 1;
  }

  for (i = 0 ; i < 5 ; i++) {
    if (0 > snprintf(digest+8*i, 8+1, "%08X", sha.Message_Digest[i])) {
      perror("@sha1_file(): snprintf() fails");
      return 1;
    }
  }
  digest[40] = 0;
    
  return 0;
}

int sha1_str(char *text, char *digest)
{
  SHA1Context sha;
  int i;
  
  SHA1Reset(&sha);
  SHA1Input(&sha, (const unsigned char*)text, strlen(text));
  if (!SHA1Result(&sha)) {
    fprintf(stderr,
	    "@sha1_str(): SHA1Result() fails.\n");
    return 1;
  }

  for (i = 0 ; i < 5 ; i++) {
    if (0 > snprintf(digest+8*i, 8+1, "%08x", sha.Message_Digest[i])) {
      perror("@sha1_file(): snprintf() fails");
      return 1;
    }
  }
  digest[40] = 0;

  return 0;
}

/* calculation with rela_path*/
int sha1_digest_via_fname_fss(const char *fname,
			      const char *root_path, char *digest)
{
  FILE *file;
  SHA1Context sha;
  int len, i;
  unsigned char buf[SHA1_BUF_LEN];
  char fullpath[MAX_PATH_LEN];
  char digest0[41], digest1[41], digest2[81];
  struct stat statbuf;

  strncpy(fullpath, fname+strlen(root_path),
	  strlen(fname)-strlen(root_path));
  fullpath[strlen(fname)-strlen(root_path)] = 0;

  if (stat(fname, &statbuf) < 0) {
    if (errno == ENOENT)
      return 2;
    else {
      perror("@sha1_digest_via_fname_fss(): stat() failed");
      return 1;
    }
  }

  SHA1Reset(&sha);

  // for dir, add a string here, just in case there is an empty file
  // with same name
  if (S_ISDIR(statbuf.st_mode)) {
    SHA1Input(&sha, (unsigned char*)"IAMDIR", 6);

    // for file, calculate normal sha1 digest
  } else {
    if (!(file = fopen(fname, "rb"))) {
      fprintf(stderr,
	      "@sha1_digest_via_fname_fss(): open %s fails: %s\n",
	      fname, strerror(errno));
      return 1;
    }

    len = fread(buf, sizeof(unsigned char), SHA1_BUF_LEN, file);
    while(len) {
      SHA1Input(&sha, buf, len);
      len = fread(buf, sizeof(unsigned char), SHA1_BUF_LEN, file);
    }

    if (fclose(file) < 0) {
      perror("sha1_digest_via_fname_fss(): fclose() failed");
      return 1;
    }
    
  }

  if (!SHA1Result(&sha)) {
    fprintf(stderr,
	    "@sha1_file(): SHA1Result() failed\n");
    return 1;
  }

  // export first stage's sha1 digest to digest0
  for (i = 0 ; i < 5 ; i++) {
    if (0 > snprintf(digest0+8*i, 8+1, "%08X", sha.Message_Digest[i])) {
      perror("@sha1_file(): snprintf() fails");
      return 1;
    }
  }
  digest0[40] = 0;

  // now calcuate relapath's sha1 digest
  char *token;
  token = strtok(fullpath, "/");
  while(token) {

    SHA1Reset(&sha);
    SHA1Input(&sha, (unsigned char*)token, strlen(token));
    if (!SHA1Result(&sha)) {
      fprintf(stderr,
	      "@sha1_file(): SHA1Result() fails.\n");
      return 1;
    }
    // export current token's sha1 digest to digest1[]
    for (i = 0 ; i < 5 ; i++) {
      if (0 > snprintf(digest1+8*i, 8+1, "%08X", sha.Message_Digest[i])) {
	perror("@sha1_file(): snprintf() fails");
	return 1;
      }
    }
    digest1[40] = 0;

    // copy first stage's digest OR previous traversal's digest to digest2[]
    if(!strncpy(digest2, digest0, 40)) {
      perror("@sha1_file(): strncpy failed");
      return 1;
    }

    // copy current token's sha1 string to digest2[]
    if(!strncpy(digest2+40, digest1, 40)) {
      perror("@sha1_file(): strncpy failed");
      return 1;
    }
    digest2[80] = 0;

    // calcuate digest2[]'s sha1 digest
    SHA1Reset(&sha);
    SHA1Input(&sha, (unsigned char*)digest2, strlen(digest2));
    if (!SHA1Result(&sha)) {
      fprintf(stderr,
	      "@sha1_file(): SHA1Result() fails.\n");
      return 1;
    }

    // export to digest0[]
    for(i = 0; i < 5; i++) {
      if (0 > snprintf(digest0+8*i, 8+1, "%08X", sha.Message_Digest[i])) {
	perror("@sha_file(): snprintf() failed");
	return 1;
      }
    }
    digest0[40] = 0;
    
    token = strtok(NULL, "/");
  }

  if (!strncpy(digest, digest0, 40)) {
    perror("@sha1_digest_via_fname_fss(): strncpy() failed");
    return 1;
  }
  digest[40] = 0;

  return 0;
}




// sha1_digest should be a char array with length=41
// hash_digest should be a char array with length=42
int compute_hash(const char *fname, const char *root_path,
		 char *sha1_digest, char *hash_digest)
{
  SHA1Context sha;
  int flag = 0; // file type flag, 0=reg file, 1=dir
  unsigned char buf[SHA1_BUF_LEN];
  char content_digest[41], path_digest[41], both_digest[81];
  struct stat statbuf;
  FILE *file = NULL;
  size_t len;

  if (stat(fname, &statbuf) < 0) {
    if (errno == ENOENT)
      return errno;
    else {
      perror("@compute_hash(): stat() failed");
      return 1;
    }
  }

  // if it is a dir, make normal sha1 string as 000...00
  if (S_ISDIR(statbuf.st_mode)) {
    flag = 1;
    SHA1Reset(&sha);
    SHA1Result(&sha);
    if (export_to_str(&sha, content_digest)) {
      fprintf(stderr, "@compute_hash(): export_to_str() failed\n");
      return 1;
    }

    // if it is regular file, calc its normal sha1 string
  }  else if (S_ISREG(statbuf.st_mode)) {
    if ((file = fopen(fname, "rb")) == NULL) {
      perror("@compute_hash(): fopen() failed");
      return 1;
    }

    SHA1Reset(&sha);
    len = fread(buf, sizeof(unsigned char), SHA1_BUF_LEN, file);
    while(len) {
      SHA1Input(&sha, buf, len);
      len = fread(buf, sizeof(unsigned char), SHA1_BUF_LEN, file);
    }

    fclose(file);
    SHA1Result(&sha);

    if (export_to_str(&sha, content_digest)) {
      fprintf(stderr, "@compute_hash(): export_to_str() failed\n");
      return 1;
    }

  } else {
    fprintf(stderr, "@compute_hash(): target neither reg file nor dir\n");
    return 1;
  }

  if (sha1_digest != NULL) {
    strncpy(sha1_digest, content_digest, 40);
    sha1_digest[40] = 0;
  }

  if (hash_digest == NULL)
    return 0;

  char relapath[MAX_PATH_LEN];
  size_t relalen = strlen(fname) - strlen(root_path);
  if(!strncpy(relapath, fname+strlen(root_path), relalen)) {
    perror("@compute_hash(): strncpy() failed");
    return 1;
  }
  relapath[relalen] = 0;


  // the following hash computation is a little bit complicated and
  // annoying, just because client on windows platform did this way
  char *token;
  token = strtok(relapath, "/");
  
   while(token) {
    SHA1Reset(&sha);
    SHA1Input(&sha, (unsigned char*)token, strlen(token));
    SHA1Result(&sha);

    if (export_to_str(&sha, path_digest)) {
      fprintf(stderr, "@compute_hash(): export_to_str() failed\n");
      return 1;
    }

    strncpy(both_digest, content_digest, 40);
    strncpy(both_digest+40, path_digest, 40);
    both_digest[80] = 0;

    SHA1Reset(&sha);
    SHA1Input(&sha, (unsigned char*)both_digest, strlen(both_digest));
    SHA1Result(&sha);

    if (export_to_str(&sha, content_digest)) {
      fprintf(stderr, "@compute_hash(): export_to_str() failed\n");
      return 1;
    }
    
    token = strtok(NULL, "/");
  }


   //  snprintf(hash_digest, 1+1, "%0X", flag);
  strncpy(hash_digest, content_digest, 40);
  hash_digest[40] = 0;

  return 0;
}



static int export_to_str(SHA1Context *sha1, char *digest_str)
{
  int i;
  for (i = 0; i < 5; i++) {
    if (snprintf(digest_str+8*i, 8+1, "%08X", sha1->Message_Digest[i]) < 0) {
      perror("@export_to_str(): snprintf() failed");
      return 1;
    }
  }
  digest_str[40] = 0;

  return 0;
}
  
  
  
  
