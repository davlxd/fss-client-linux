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

#include "wrap-sha1.h"

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
  struct stat statbuf;

  strncpy(fullpath, fname+strlen(root_path),
	  strlen(fname)-strlen(root_path));
  fullpath[strlen(fname)-strlen(root_path)] = 0;


  if (stat(fname, &statbuf) < 0) {
    if (errno == ENOENT)
      return 2;
    else {
      perror("@sha1_digest_via_fname_fss(): stat() failed\n");
      return 1;
    }
  }

  /* following code is the way to calc dir's sha1 digest */
  if (S_ISDIR(statbuf.st_mode)) {
    SHA1Reset(&sha);

    char *token;
    token = strtok(fullpath, "/");
    while(token) {
      SHA1Input(&sha, (unsigned char*)token, strlen(token));
      token = strtok(NULL, "/");
    }

    /* make sure dir's sha1 string is different from an empty file in
     * same name */
    SHA1Input(&sha, (unsigned char*)"IAMDIR", 6);
  
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
  /* end */
  
  if (!(file = fopen(fname, "rb"))) {
    fprintf(stderr,
	    "@sha1_digest_via_fname_fss(): open %s fails: %s\n",
	    fname, strerror(errno));
    return 1;
  }
  
  SHA1Reset(&sha);

  len = fread(buf, sizeof(unsigned char), SHA1_BUF_LEN, file);
  while(len) {
    SHA1Input(&sha, buf, len);
    len = fread(buf, sizeof(unsigned char), SHA1_BUF_LEN, file);
  }

  char *token;
  token = strtok(fullpath, "/");
  while(token) {
    SHA1Input(&sha, (unsigned char*)token, strlen(token));
    token = strtok(NULL, "/");
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



  if (0 != fclose(file)) {
    perror("@sha1_digest_via_fname_fss(): fclose() fails");
    return 1;
  }

  return 0;
}

  
  
  
  
