/*
 * main function
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

#include "files.h"
#include "sock.h"
#include "params.h"
#include "protocol.h"
#include "wrap-inotify.h"
#include <stdlib.h>
#include <stdio.h>



int main(int argc, char **argv)
{

  char server[VALUE_LEN];
  char path[VALUE_LEN];
  int monitor_fd;
  int sockfd;

  setbuf(stdout, NULL);
  memset(path, 0, VALUE_LEN);
  memset(server, 0, VALUE_LEN);


  if (get_param_value("Path", path)) {
    perror("@main(): get_param_value Path fails\n");
    exit(0);
  }
  if (!strlen(path)) {
    perror("Path in fss.conf cannot be empty");
    exit(0);
  }
  if (get_param_value("Server", server)) {
    perror("@main(): get_param_value Server fails\n");
    exit(0);
  }
  if (!strlen(server)) {
    perror("Server in fss.conf cannot be empty");
    exit(0);
  }

  if (set_rootpath(path)) {
    fprintf(stderr, "@main(): set_rootpath() failed\n");
    return 1;
  }

  if (update_files()) {
    fprintf(stderr,
	    "@main(): update_files() failed\n");
    return 1;
  }
  
  if (monitors_init(path,
		    IN_MODIFY|IN_CREATE|IN_DELETE|
		    IN_MOVED_FROM|IN_MOVED_TO, &monitor_fd)) {
    fprintf(stderr, "@main(): monitors_init() fails\n");
    exit(0);
  }
  if (fss_connect(server, &sockfd)) {
    fprintf(stderr, "@main(): fss_connect() fails\n");
    exit(0);
  }

  if (client_polling(monitor_fd, sockfd)) {
    fprintf(stderr, "@main(): client_polling() failed\n");
    return 1;
  }
  


	

  return 0;
}
