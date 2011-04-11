/*
 * network manipulate functions
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

#include "sock.h"

int fss_connect(const char *text, int *sockfd)
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

  if ((*sockfd = socket(result->ai_family, result->ai_socktype,
			result->ai_protocol)) < 0) {
    perror("@fss_connect(): socket() fails");
    return 1;
  }
  
  /* only deal with the first addr */
  if ((rv = connect(*sockfd, result->ai_addr, result->ai_addrlen)) < 0) {
    perror("@fss_connect(): connect() fails");
    return 1;
  }

  freeaddrinfo(result);
  return 0;
}
