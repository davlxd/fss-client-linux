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

#include "checksum.h"
#include <inttypes.h>
#include "wrap-sha1.h"

extern int errno;

uint32_t rolling_checksum(char *buf1, int32_t len)
{
    int32_t i;
    uint32_t s1, s2;
    char *buf = (char *)buf1;

    s1 = s2 = 0;
    for (i = 0; i < (len-4); i+=4) {
      s2 += 4*(s1 + buf[i]) + 3*buf[i+1] + 2*buf[i+2] + buf[i+3];
	s1 += (buf[i+0] + buf[i+1] + buf[i+2] + buf[i+3]);
    }
    for (; i < len; i++) {
	s1 += (buf[i]); s2 += s1;
    }
    return (s1 & 0xffff) + (s2 << 16);
}

int send_blk_checksums(int sockfd, const char *pathname,
		       const char *relaname, 
		       off_t block_size,
		       const char *prefix)
{
  FILE *in;
  size_t len;
  struct stat sb;
  off_t block_num;
  off_t remainder;
  char buf[block_size];
  char record[MAX_PATH_LEN];

  if (stat(pathname, &sb) < 0) {
    perror("@rolling_files(): stat() failed");
    return 1;
  }

  block_num = sb.st_size/block_size;
  remainder = sb.st_size % block_size;
  if (remainder)
    block_num++;

  //TODO:printf off_t as unsigned long long here

  len = snprintf(record, MAX_PATH_LEN,
		 "%s\n" \
		 "%"PRIu64"\n" \
		 "%"PRIu64"\n" \
		 "%"PRIu64"\n" \
		 "%"PRIu64"\n" \
		 "%s\n" \
		 "\n",
		 prefix,
		 (uint64_t)sb.st_size,
		 (uint64_t)block_size,
		 (uint64_t)block_num,
		 (uint64_t)remainder,
		 relaname
		 );

  if (write(sockfd, record, len) < 0) {
    perror("@send_checksums(): write() to sockfd failed");
    return 1;
  }
  
  if ((in = fopen(pathname, "rb")) == NULL) {
    perror("@send_checksums(): fopen() intput file failed");
    return 1;
  }

  len = fread(buf, sizeof(char), block_size, in);
  while(len) {

    char sha1[41];
    sha1_digest_of_buffer(buf, len, sha1);
    len = snprintf(record, MAX_PATH_LEN, "%08"PRIX32"\t%s\n",
		   rolling_checksum(buf, (int32_t)len), sha1);

    if (write(sockfd, record, len) < 0) {
      perror("@send_checksums(): write() failed");
      return 1;
    }
    
    len = fread(buf, sizeof(char), block_size, in);
  }

  fclose(in);
  return 0;
}


  
  
  
