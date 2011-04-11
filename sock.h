/*
 * network manipulate functions, header file
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
#ifndef _SOCK_H_
#define _SOCK_H_

#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>

extern int errno;


#define PORT 3375
#define PORT_STR "3375"

#ifndef BUF_LEN
#define BUF_LEN 4096
#endif

#define CLIENT_CHANGE "A"

#define SERVER_REQUEST_SHA1_FILE_SZ "a"
#define SERVER_REQUEST_SHA1_FILE "b"

#define SERVER_QUIET 0
#define CHANGE_SENT 1
#define SHA1_FILE_SZ_SENT 2
#define SHA1_FILE_SENT 3

/* input IP addr or Domain name AS text */
int fss_connect(const char *text, int *sockfd);

#endif
