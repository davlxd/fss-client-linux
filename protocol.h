/*
 * core, header file, client
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

#ifndef _FSS_PROTOCOL_H_
#define _FSS_PROTOCOL_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/select.h>
#include <sys/time.h>
#include "files.h"

extern int errno;

#ifndef BUF_LEN
#define BUF_LEN 4096
#endif

#ifndef MAX_PATH_LEN
#define MAX_PATH_LEN 1024
#endif

#define CLI_REQ_SHA1_FSS "A"
#define CLI_REQ_FILE "B"
#define SER_REQ_FILE "D"
#define SER_REQ_DEL_IDX "E"
#define SER_RECEIVED "F"
#define DONE "G"
#define SHA1_FSS_INFO "H"
#define LINE_NUM "I"
#define FILE_INFO "J"
#define DEL_IDX_INFO "K"
#define FIN "L"
#define CLI_REQ_SHA1_FSS_INFO "M"
#define DIR_INFO "N"



#define WAIT_SHA1_FSS_INFO 1
#define WAIT_SHA1_FSS 3
#define WAIT_ENTRY_INFO 5
#define WAIT_FILE 7
#define WAIT_MSG_SER_REQ_FILE 9
#define WAIT_MSG_SER_RECEIVED 11
#define WAIT_MSG_SER_REQ_DEL_IDX 13

static int status;

static int sockfd;
static int monifd;

// monitor lock 
static int lock;

static char rela_name[MAX_PATH_LEN];
static time_t mtime;
static off_t req_sz;


int client_polling(int moni_fd, int sock_fd);
static int handle_monifd();
static int handle_sockfd();
static int status_WAIT_SHA1_FSS_INFO();
static int status_WAIT_SHA1_FSS();
static int status_WAIT_ENTRY_INFO();
static int status_WAIT_FILE();
static int status_WAIT_MSG_SER_REQ_FILE();
static int status_WAIT_MSG_SER_RECEIVED();
static int status_WAIT_MSG_SER_REQ_DEL_IDX();

static int analyse_sha1();
static int download_sync();
static int receive_line(int sockfd, char *text, int len);
static int set_fileinfo(char *buf);




#endif
