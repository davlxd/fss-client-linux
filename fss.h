/*
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
#include <stdarg.h>

#define INCLUDE_HIDDEN 0

#define BUF_LEN 4096
#define MAX_PATH_LEN 1024

#define BLOCK_LEN 700

// length of record of hash.fss
#define HASH_LEN 40

#define PORT 3375
#define PORT_STR "3375"

#define FSS_DIR ".fss"


#define CLI_REQ_HASH_FSS "A"
#define CLI_REQ_FILE "B"
#define SER_REQ_FILE "D"
#define SER_REQ_DEL_IDX "E"
#define SER_RECEIVED "F"
#define DONE "G"
#define HASH_FSS_INFO "H"
#define LINE_NUM "I"
#define FILE_INFO "J"
#define DEL_IDX_INFO "K"
#define FIN "L"
#define CLI_REQ_HASH_FSS_INFO "M"
#define DIR_INFO "N"
#define BLK_CHKSUM "O"
#define CLI_REQ_BLK_CHKSUM "P"
#define BLK "Q"
#define CLI_REQ_BLK "R"
#define SER_RECEIVED_BLK "T"

/* From the perspective of client */
enum direction {
  UPLOAD = 0,
  DOWNLOAD = 1
};



#define MIN(a,b) ((a)<(b)?(a):(b))
#define MAX(a,b) ((a)>(b)?(a):(b))

#define CUR_POS(fd) lseek(fd, 0, SEEK_CUR)


int rela2full(const char *relapath, char *fullpath, size_t size);
int full2rela(const char *fullpath, char *relapath, size_t size);

off_t curpos(int fd);

int fss_connect(const char *text, int *fd);
int fss_readline(int fd, char *buf, size_t size);
int read_msg_head(int fd, char *buf, size_t size);
int fss_write(int fd, size_t size, char *fmt, ...);



#endif
