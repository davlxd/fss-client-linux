/*
 * functions frequently used by other modules
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

#ifndef _F_SS_H_
#define _F_SS_H_

#define _XOPEN_SOURCE 500

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/inotify.h>
#include <signal.h>
#include <fcntl.h>
#include <ftw.h>
#include <dirent.h>
#include <stdint.h>
#include <inttypes.h>
#include <netdb.h>

#define INCLUDE_HIDDEN 0

#define BUF_LEN 4096
#define MAX_PATH_LEN 1024

#define BLOCK_LEN 700

// length of record of hash.fss
#define HASH_LEN 40

#define PORT 3375
#define PORT_STR "3375"

#define FSS_DIR ".fss"


#define MIN(a,b) ((a)<(b)?(a):(b))
#define MAX(a,b) ((a)>(b)?(a):(b))


#endif
