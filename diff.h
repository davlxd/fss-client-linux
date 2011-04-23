/*
 * Linearly diff match algorithm, header file
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
#ifndef _DIFF_H_
#define _DIFF_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SHA1_LINE_LEN 42

#ifndef MAX_LINE_LEN
#define MAX_LINE_LEN SHA1_LINE_LEN
#endif

FILE *file_in_0;
FILE *file_in_1;
FILE *file_out_0;
FILE *file_out_1;
FILE *file_out_2;

/* fout0 -> lines exist in fin0, not in fin1
 * fout1 -> lines exist in fin1, not in fin0
 * fout2 -> lines both in fin0 and fin1
 */
int diff(const char *fin0, const char *fin1,
	 const char *fout0, const char *fout1, const char *fout2);



#endif
