/*
 * Read fss.conf file header file
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

#ifndef _PARAMS_H_
#define _PARAMS_H_

//#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <sys/stat.h>

#ifndef PARAM_LEN
#define PARAM_LEN 16
#endif
#ifndef VALUE_LEN
#define VALUE_LEN 1024
#endif

extern int errno;

static const char *conf_fname = "fss.conf";
static FILE *conf_fp;

int get_param_value(const char *k, char *v);
static int open_file(FILE **conf_fp);
static int skip_space(FILE *conf_fp);
static int skip_comment(FILE *conf_fp);
static int parse(FILE *conf_fp, const char *k, char *v);
static int parse_param(FILE *, char c,  const char *k, char *v);



#endif
