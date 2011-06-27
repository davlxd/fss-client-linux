/*
 * hashtable.h
 *
 * This module generate a hashtable based on checksums of blocks of a
 * file server sent
 * Some code comes from rsync-3.0.8 which is also licensed under GPL
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

#ifndef _FSS_HASH_TABLE_H_
#define _FSS_HASH_TABLE_H_

#include "fss.h"

typedef struct block_struct
{
  uint64_t offset;
  uint64_t block_len;

  uint32_t checksum0;
  char checksum1[HASH_LEN+1];
  
  int32_t chain;
    
} block_struct;


typedef struct file_struct
{
  uint64_t file_size;
  uint64_t block_size;
  uint64_t block_num;
  uint64_t remainder;
  char relaname[MAX_PATH_LEN];
  
  block_struct *bstructs;
  
} file_struct;

int receive_blk_checksums(file_struct **f_s);
int build_hashtable(int32_t **hashtable);
int32_t search_hashtable(uint32_t sum0);
void free_hashtable();

void check_blk_structs();


#endif

