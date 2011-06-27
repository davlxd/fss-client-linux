/*
 * hashtable.c
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

#include "hashtable.h"
#include <inttypes.h>

extern int errno;

static file_struct *fstruct;
static int32_t *hash_table;
static uint32_t table_sz;

#define TRADITIONAL_TABLESIZE (1<<16)
#define SUM2HASH2(s1, s2) (((s1) + (s2)) & 0xFFFF)
#define SUM2HASH(sum) SUM2HASH2((sum)&0xFFFF, (sum)>>16)


/* I didn't use temporary variable to enhance readability,
 * so this function may looks a little bit messy at your first glance */
int receive_blk_checksums(file_struct **filestruct)
{
  char buf[MAX_PATH_LEN];

  if (read_msg_head(buf, MAX_PATH_LEN)) {
    fprintf(stderr, "@receive_blk_checksums(): read_msg_head() failed\n");
    return 1;
  }

  fstruct = (file_struct *)calloc(1, sizeof(file_struct));
  if (!fstruct) {
    perror("@receive_blk_checksums(): calloc() failed");
    return 1;
  }
  if (filestruct != NULL)
    *filestruct = fstruct;

  if (sscanf(buf,
	     "%"PRIu64"\n" \
	     "%"PRIu64"\n" \
	     "%"PRIu64"\n" \
	     "%"PRIu64"\n" \
	     "%s\n", 
	     &fstruct->file_size,
	     &fstruct->block_size,
	     &fstruct->block_num,
	     &fstruct->remainder,
	     fstruct->relaname)
      == EOF) {
    perror("@receive_blk_checksums(): sscanf() failed");
    return 1;
  }

  fstruct->bstructs = (block_struct *)calloc(fstruct->block_num, sizeof(block_struct));
  if (!fstruct) {
    perror("@receive_blk_checksums(): calloc() failed");
    return 1;
  }

  uint64_t i;
  for (i = 0; i < fstruct->block_num; i++ ) {
    
    (fstruct->bstructs + i)->offset = i * fstruct->block_size;
    
    if (i == fstruct->block_num -1 && fstruct->remainder)
      (fstruct->bstructs + i)->block_len = fstruct->remainder;
    else
      (fstruct->bstructs + i)->block_len = fstruct->block_size;

    if (fss_readline(buf, MAX_PATH_LEN)) {
      fprintf(stderr, "@receive_blk_checksums(): fss_readline() failed\n");
      return 1;
    }
    if (sscanf(buf,
	       "%08"PRIX32"\t%s\n", 
	       &(fstruct->bstructs + i)->checksum0,
	       (fstruct->bstructs + i)->checksum1)
	== EOF) {
      perror("@receive_blk_checksums(): sscanf() failed");
      return 1;
    }
     
  }
  
  return 0;
}


int build_hashtable(int32_t **hashtable)
{
  int i;

  table_sz = (uint32_t)(fstruct->block_num / 8) * 10 + 11;
  if (table_sz < TRADITIONAL_TABLESIZE)
    table_sz = TRADITIONAL_TABLESIZE;
  
  hash_table = calloc(table_sz, sizeof(int32_t));
  if (!hash_table) {
    perror("@build_hashtable(): calloc() failed");
    return 1;
  }

  memset(hash_table, 0xFF, table_sz * sizeof(hash_table[0]));

  if (table_sz == TRADITIONAL_TABLESIZE) {
    for (i = 0; i < fstruct->block_num; i++) {
      uint32_t t = SUM2HASH((fstruct->bstructs + i)->checksum0);
      (fstruct->bstructs + i)->chain = hashtable[t];
      hashtable[t] = i;
    }
  } else {
    for (i = 0; i < fstruct->block_num; i++) {
      uint32_t t = (fstruct->bstructs + i)->checksum0 % table_sz;
      (fstruct->bstructs + i)->chain = hashtable[t];
      hashtable[t] = i;
    }
  }
  

  if (hashtable != NULL)
    *hashtable = hash_table;
  
  

  return 0;
}


int32_t search_hashtable(uint32_t sum0)
{
  
  if (table_sz == TRADITIONAL_TABLESIZE)
    return hashtable[SUM2HASH(sum0)];
  else
    return hashtable[sum0 % table_sz];

}
    

void free_hashtable()
{
  free(fstruct->bstructs);
  free(fstruct);
  fstruct = NULL;

  free(hash_table);
  hash_table = NULL;

}

void check_blk_structs()
{
  printf("fstruct->file_size: --%"PRIu64"--\n", fstruct->file_size);
  printf("fstruct->block_size: --%"PRIu64"--\n", fstruct->block_size);
  printf("fstruct->block_num: --%"PRIu64"--\n", fstruct->block_num);
  printf("fstruct->remainder: --%"PRIu64"--\n", fstruct->remainder);
  printf("fstruct->relaname: --%s--\n", fstruct->relaname);

  uint64_t i;
  for (i = 0; i < fstruct->block_num; i++) {
    printf("(fstruct->bstructs + %"PRIu64")->offset: --%"PRIu64"--\n", i, 
	   (fstruct->bstructs + i)->offset);
    printf("(fstruct->bstructs + %"PRIu64")->block_len: --%"PRIu64"--\n", i, 
	   (fstruct->bstructs + i)->block_len);
    printf("(fstruct->bstructs + %"PRIu64")->checksum0: --%08"PRIX32"--\n",
	   i, (fstruct->bstructs + i)->checksum0);
    printf("(fstruct->bstructs + %"PRIu64")->checksum1: --%s--\n", i, 
	   (fstruct->bstructs + i)->checksum1);

  }

}

