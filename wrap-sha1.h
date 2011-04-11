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
#ifndef _WRAP_SHA1_H_
#define _WRAP_SHA1_H_

#include "sha1.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
extern int errno;

#ifndef SHA1_BUF_LEN
#define SHA1_BUF_LEN 1024
#endif

#ifndef MAX_PATH_LEN
#define MAX_PATH_LEN 1024
#endif


int sha1_digest_via_fname(const char *fname, char *digest);

/* I need this function because of my dirty design
 * this function ask for root_path to calculate sha1 including
 * relative path*/
int sha1_digest_via_fname_fss(const char *fname,
			      const char *root_path, char*digest);

int sha1_file(FILE *file, char *digest);
int sha1_str(char *text, char *digest);




#endif

  
  
  
    
    
