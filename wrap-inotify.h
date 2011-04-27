/*
 * wrap inotify, mainly implement recursively monitoring, header file
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

#ifndef _WRAP_INOTIFY_H_
#define _WRAP_INOTIFY_H_


//#define FIFO0 "/tmp/fifo-wrap-inotify.0"
//#define FIFO1 "/tmp/fifo-wrap-inotify.1"

extern int errno;

// monitoring STRUCT per directory
typedef struct monitor
{
  char *pathname;
  int fd;
  int wd;

  struct monitor *p;  //previous one
  struct monitor *n;  //next one, as u c, it is a ddlink
} monitor;



/* API */
int monitors_init(const char *root_path, uint32_t mask, int *fd);
int monitors_cleanup();


#endif
