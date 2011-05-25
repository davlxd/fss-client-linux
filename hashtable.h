/*
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
#ifndef _FSS_HASH_TABLE_H_
#define _FSS_HASH_TABLE_H_

#include "fss.h"

typedef struct block_struct
{
  size_t idx;
  size_t chain;
  

  
} block_struct;


typedef struct file_struct
{
  off_t len;
  block_struct *blocks;
  



} file_struct;


int init_hashtable_from_file(const char *t_file, file_struct *f_s);
int build_hashtable(file_struct *f_s);
int cleanup_hashtable(file_struct *f_s);



#endif

  
  
  
    
    
