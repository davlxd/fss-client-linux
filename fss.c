/*
 * function implement
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
#include "fss.h"

extern int errno;

char rootpath[MAX_PATH_LEN];
static int sockfd;

//strip '/' in the end
int update_rootpath(const char *path)
{
  struct stat statbuf;
  size_t strncpy_len, path_len;
  char *ptr;

  if (stat(path, &statbuf) < 0) {
    perror("@update_rootpath(): stat() failed");
    return 1;
  }

  path_len = strlen(path);
  strncpy_len = path[path_len-1] == '/' ? path_len-1 : path_len;

  if (!strncpy(rootpath, path, strncpy_len)) {
    fprintf(stderr, "@update_rootpath(): strncpy() failed\n");
    return 1;
  }
  rootpath[++strncpy_len] = 0;
  
  return 0;
  
}

int rela2full(const char *relapath, char *fullpath, size_t size)
{

  size_t len;
  len = snprintf(fullpath, size, "%s/%s", rootpath, relapath);

  if (len == size)
    fprintf(stderr, "@rela2full(): pathname too long !!!");
  
  
  return 0;
}


int full2rela(const char *fullpath, char *relapath, size_t size)
{
  size_t len;
  len = strlen(fullpath) - strlen(rootpath);
  if (len >= size)
    fprintf(stderr, "@full2rela(): pathname too long !!!");
  
  // omit '/' between rootpath and relapath
  if (!strncpy(relapath, fullpath + strlen(rootpath) + 1, len)) {
    perror("@full2rela(): strncpy() failed");
    return 1;
  }

  return 0;
}


int fss_connect(const char *text, int *fd)
{
  struct addrinfo hints, *result;
  int rv;

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_socktype = SOCK_STREAM;

  if ((rv = getaddrinfo(text, PORT_STR, &hints, &result)) != 0) {
    fprintf(stderr,
	    "@fss_connect(): getaddrinfo() %s, %s fails: %s\n",
	    text, PORT_STR, gai_strerror(rv));
    return 1;
  }

  if ((*fd = socket(result->ai_family, result->ai_socktype,
			result->ai_protocol)) < 0) {
    perror("@fss_connect(): socket() fails");
    return 1;
  }

  sockfd = *fd;
  
  /* only deal with the first addr */
  if ((rv = connect(*fd, result->ai_addr, result->ai_addrlen)) < 0) {
    perror("@fss_connect(): connect() fails");
    return 1;
  }

  freeaddrinfo(result);
  return 0;
}


/* This function comes from UNP v3 with a little modification
 * without appending LF to buf */
int fss_readline(char *buf, size_t size)
{
  char c;
  char *ptr;
  size_t i;
  int rv;

  ptr = buf;
  for(i = 1; i < size; i++) {
    if ((rv = read(sockfd, &c, 1)) == 1) {
      if (c == '\n')
	break;
      *ptr++ = c;
      
    } else if (rv == 0) {
      *ptr = 0;
      return 0;
      
    } else {
      if (errno == EINTR) {
	i--;
	continue;
      }

      perror("@fss_readline(): read failed");
      return 1;
    }
    
  }// end for(..

  *ptr = 0;
  return 0;
}


/* There are 2 LF between msg head and body, this function strip second
 * LF and so only append one LF to buf */
int read_msg_head(char *buf, size_t size)
{
  char c, d;
  char *ptr;
  size_t i;
  int rv;

  ptr = buf;
  c = d = 0;
  for(i = 1; i < size; i++) {
    c = d;
    if ((rv = read(sockfd, &d, 1)) == 1) {
      if (c == '\n' && d == '\n')
	break;
      *ptr++ = d;
      
    } else if (rv == 0) {
      *ptr = 0;
      return 0;
      
    } else {
      if (errno == EINTR) {
	d = c;
	i--;
	continue;
      }

      perror("@fss_readline(): read failed");
      return 1;
    }
    
  }// end for(..

  *ptr = 0;
  return 0;
}


int fss_write(size_t size, char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);

  char msg[size];
  vsnprintf(msg, size, fmt, ap);

  if(write(sockfd, msg, strlen(msg)) < 0) {
    perror("@fss_send(): write() failed");
    return 1;
  }

  va_end(ap);

  return 0;
}
