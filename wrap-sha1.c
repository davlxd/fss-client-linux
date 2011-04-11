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
  char digest0[41], digest1[41], digest2[81];
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

  // now calcuate relapth's sha1 digest
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
    SHA1Input(&sha, digest2, strlen(digest2));
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

  
  
  
  
