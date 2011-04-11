/*
 * Log on functions header file
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
#ifndef _LOG_H_
#define _LOG_H_

#include <stdio.h>
#include <time.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <syslog.h>
#include <string.h>
#include <unistd.h>

#ifndef LOG_RECORD_MAX_LEN
#define LOG_RECORD_MAX_LEN 1024
#endif

static FILE *log_fp = NULL;
static const char *log_fname = "fss.log";

void fsslog(const char *msg, ...);
static void logit(const char *fmt, va_list ap);


#endif
