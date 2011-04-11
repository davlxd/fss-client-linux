/*
 * funtions frequently used by other modules
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

#include "fss.h"


int verify_path(const char *path)
{
  struct stat statbuf;
  
  if (!path) {
    fprintf(stderr, "@verify_path(): path is NULL, verify fails\n");
    return 1;
  } else if (stat(path, &statbuf) < 0) {
    if (errno == ENOENT)
      perror("@verify_path(): no such path");
    else
      perror("@verify_path(): stat error");
    return 1;
  }

  if (!S_ISDIR(statbuf.st_mode)) {
    fprintf(stderr, "@verify_path(): path -%s- is valid, "\
	    "but not a dir\n", path);
    return 1;
  }

  return 0;
}


/* turn  /foo/fee/ into  /foo/fee
 */
int strip_path(const char *path, char *new_path)
{
  if (!path) {
    fprintf(stderr, "@strip_path(): strip fails, path is NULL\n");
    return 1;
  }
  if (path[strlen(path)-1] == '/') {
    strncpy(new_path, path, strlen(path)-1);
    new_path[strlen(new_path)] = 0;
    return 0;
  } else {
    strncpy(new_path, path, strlen(path));
    new_path[strlen(path)] = 0;
    return 0;
  }
  return 0;
}


/* replacement for ftw() and derived from APUE
 * 
 * path -> root path
 * *f -> handle function pointer
 * ifreg -> if handle regular files
 * ifdir -> if handle directories
 * iflnk -> if follow links
 * ifhidden -> if not ignore hidden file/dirs
 *
 * other file types such as socket, FIFO are not included
 */
int deepin(char *path, int (*f)(const char *),
		  int ifreg, int ifdir, int iflnk, int ifhidden)
{
  struct stat statbuf;
  DIR *dp;
  struct dirent *dep;
  char *ptr;

  errno = 0;
  /* Due to_files() is immediately called after file system changed,
   * so file may not be there*/
  if (stat(path, &statbuf) < 0 && errno != ENOENT) {
    fprintf(stderr,
	    "@deepin(): failed to stat %s: %s\n",
	    path, strerror(errno));
    return 1;
  }
  if (errno == ENOENT)
    return 0;
  
  if (S_ISCHR(statbuf.st_mode) || S_ISBLK(statbuf.st_mode) ||
      S_ISFIFO(statbuf.st_mode) || S_ISSOCK(statbuf.st_mode))
    return 0;
  if (S_ISREG(statbuf.st_mode) && !ifreg)
    return 0;
  if (S_ISLNK(statbuf.st_mode) && !iflnk)
    return 0;
  
  if (!S_ISDIR(statbuf.st_mode) || ifdir) {
    if (f(path)) {
      fprintf(stderr,
	      "@deepin(): monitor_connect(%s) fail\n", path);
      return 1;
    }
  }

  if (S_ISDIR(statbuf.st_mode)) {
    
    ptr = path + strlen(path);
    *ptr++ = '/';
    *ptr = 0;

    if ((dp = opendir(path)) == NULL) {
      fprintf(stderr,
	      "@deepin(): fail to opendir %s. %s\n",
	      path, strerror(errno));
      return 1;
    }

    while ((dep = readdir(dp)) != NULL) {
      if (strcmp(dep->d_name, ".") == 0 ||
	  strcmp(dep->d_name, "..") == 0 )
	continue;
      if (strncmp(dep->d_name, ".", 1) == 0 && !ifhidden)
	continue;
      if (strncmp(dep->d_name, FSS_DIR, strlen(FSS_DIR)) == 0)
	continue;
      if (strncpy(ptr, dep->d_name,
		  strlen(dep->d_name)) == NULL) {
	fprintf(stderr,
		"@deepin(): fail to strncpy %s to %s %s\n",
		dep->d_name, path, strerror(errno));
	return 1;
      }
      *(ptr+strlen(dep->d_name)) = 0;

      if (deepin(path, f, ifreg, ifdir, iflnk, ifhidden)) {
	fprintf(stderr,
		"@deepin(): recurse deepin(%s) error\n",
		path);
	return 1;
      }
    }
    if (closedir(dp) < 0) {
      fprintf(stderr,
	      "@deepin(): fail to closedir() %s\n",
	      path);
      return 1;
    }
  }

  return 0;
}
