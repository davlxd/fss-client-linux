/*
 * match.c
 *
 * Slice target file to overlapped blocks and check if there is a match
 * based on hashtable generated in hashtable.c
 * This is just a simple version of rsync algorithm
 *
 * Copyright (c) 2010, 2011 lxd <i@lxd.me>
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

#include "match.h"
#include "checksum.h"
#include "fss.h"

extern int errno;

static int fd;
static char *block;

static int read_block(off_t stride, off_t size);

static int send_block();
static int send_block_req();
static int send_done();


int block_match(file_struct *fstruct, int32_t *hash_table, direction dt)
{
  char fullpath[MAX_PATH_LEN];
  uint32_t sum0;
  int32_t i;
  off_t file_size;
  struct stat sb;
  uint64_t end_pos0, end_pos1;

  // if the file on client's side is deleted, truncated and make its
  // size smaller than block size or whatever during sync time, 
  // just send DONE and we will handle it in following recursive
  // sync stages.
  rela2full(fstruct->relaname, fullpath);
  if (stat(fullpath, &sb) < 0) {
    if (errno == ENOENT) {
      send_done();
      return 0;
      
    } else {
      perror("@block_match(): stat() failed");
      return 1;
    }
  }

  file_size = sb.st_size;
  if (file_size <= fstruct->block_size) {
    send_done();
    return 0;
  }
  block_size = fstruct->block_size;
  
  fd = open(fullpath, O_RDONLY);
  if (fd == -1) {
    perror("block_match(): open() failed");
    return 1;
  }
    
  end_pos0 = file_size - block_size;
  end_pos1 = file_size - fstruct->remainder;

  while( CUR_POS(fd) <= end_pos) {

    

    if (read_block(0)) {
      fprintf(stderr, "@block_match(): read_block() failed\n");
      return 1;
    }

    sum0 = rolling_checksum(block, block_size);
    i = search_hashtable(sum0);

  }
  

  



  
  
  close(fd);
  return 0;
}


static int read_block(off_t stride, off_t size)
{
  ssize_t len;

  // TODO: normally size is block size, when read remainder part
  // of the file, size will be smaller, so if-logic here is not appropriate
  if (stride == 0 || stride >= size) {
    len = read(fd, block, size);
    if (len == -1) {
      perror("@read_block(): read() failed");
      return 1;
    }
    
  } else {
    memmove(block, block+stride, block_num-stride);
    if ((len = read(fd, block + (block_num-stride), stride)) < 0) {
      perror("@read_block(): read() failed");
      return 1;
    }
    
  }
    
  return 0;
}
