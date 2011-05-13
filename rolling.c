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

#include "rolling.h"

extern int errno;


uint32_t get_checksum1(char *buf1, int32_t len)
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

int rolling_file(const char *in_path, const char *out_path)
{
  FILE *in, *out;
  size_t len;
  int len1;
  char buf[BLOCK_LEN];
  char record[RECORD_LEN+1];

  if ((in = fopen(in_path, "rb")) == NULL) {
    perror("@rolling_file(): fopen() intput file failed");
    return 1;
  }

  if ((out = fopen(out_path, "w+")) == NULL) {
    perror("@rolling_file(): fopen() output file failed");
    return 1;
  }

  len = fread(buf, sizeof(char), BLOCK_LEN, in);
  while(len) {
    len = snprintf(record, RECORD_LEN+1, "%08"PRIX32"\n",
		   get_checksum1(buf, (int32_t)len));
    record[len] = 0;

    if (EOF == (fputs(record, out))) {
      perror("@rolling_file(): fputs() failed");
      return 1;
    }

    len = fread(buf, sizeof(char), BLOCK_LEN, in);
  }

  fflush(out);
  fclose(in);
  fclose(out);
  
  return 0;
}


  
  
  
