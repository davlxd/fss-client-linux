/*
 * wrap inotify, mainly implement recursively monitoring, inotify mask
 * is passed in as argument.
 *
 * symbolic links not ignored.
 *
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
#include "wrap-inotify.h"

static char FIFO0[MAX_PATH_LEN];
static char FIFO1[MAX_PATH_LEN];

static char fullpath[MAX_PATH_LEN];
static fd_set set;

/* all sub-dirs gonna be monitored under same inotify mask */
static uint32_t mask;

/* point to current monitor of ddlink,
 * after recurse iterator, it point to tail of list */
static monitor *tail_monitor;


static int rfd, wfd;
static pid_t pid;
static int maxfd;


static int monitors_poll();
static void sig_child(int);

static int join_fname(const monitor*, const char*);
static int monitor_connect(const char*, const struct stat*, int,
			   struct FTW*);
static int monitor_connect_via_fpath(const monitor*, const char*);
static int monitor_disconnect(monitor*);
static int monitor_disconnect_via_fpath(const monitor*, const char*);

/*
 * arg:root_path -> root directory we monitor, pass in
 * arg:mask -> inotify mask
 * arg:fd -> file descriptor created, pass out
 *
 * return:1 -> error
 * return:0 -> ok
 */
int monitors_init(const char *root_path, uint32_t m, int *fd)
{
  struct sigaction act, oact;
  act.sa_handler = sig_child;
  sigemptyset(&act.sa_mask);
  act.sa_flags = 0;
  if ( 0 > sigaction(SIGCHLD, &act, &oact)) {
    perror("@monitors_init() @parent: sigaction fails");
    return 0;
  }

  tmpnam(FIFO0);
  tmpnam(FIFO1);
  /* establish FIFO */
  if ((0 > mkfifo(FIFO0, S_IWUSR | S_IRUSR | S_IRGRP | S_IWGRP))
       && (errno != EEXIST)) {
    perror("@monitors_init(): mkfifo0 fails");
    return 1;
  }
  if ((0 > mkfifo(FIFO1, S_IWUSR | S_IRUSR | S_IRGRP | S_IWGRP))
       && (errno != EEXIST)) {
    perror("@monitors_init(): mkfifo1 fails");
    return 1;
  }

  /* fork routine */
  if ((pid = fork()) < 0) {
    perror("@monitors_init(): fork() fails");
    return 0;
   } else if (pid > 0) {  //parent

    /* open FIFO, parent and child should following same order
     * in case deadlock */
    if (0 > (rfd = open(FIFO0, O_RDONLY, 0))) {
      perror("@monitors_init() @parent: open fifo0 for read fails");
      return 1;
    }
    if (0 > (wfd = open(FIFO1, O_WRONLY, 0))) {
      perror("@monitors_init() @parent: ppen fifo1 for write fails");
      return 1;
    }
    /* pass out fd */
    *fd = rfd;
    return 0;
    
  } else {  // child 
    
    /* first FIFO0, second FIFO1, order is important here */
    if (0 > (wfd = open(FIFO0, O_WRONLY, 0))) {
      perror("@monitors_init() @child: open fifo0 for write fails");
      exit(1);
    }
    if (0 > (rfd = open(FIFO1, O_RDONLY, 0))) {
      perror("@monitors_init() @child: open fifo1 for read fails");
      exit(1);
    }

    /* copy write fd to standard output */
    if (wfd != STDOUT_FILENO) {
      if (dup2(wfd, STDOUT_FILENO) != STDOUT_FILENO) {
    	perror("@monitors_init(), @child: dup2() fails");
    	exit(1);
      }
      close(wfd);
    }

    mask = m;
    tail_monitor = NULL;
    FD_ZERO(&set);
    FD_SET(rfd, &set);

    if (nftw(root_path, monitor_connect, 10, FTW_DEPTH) != 0) {
      perror("@monitor_init() @child: ftw failed");
      return 1;
    }

    tail_monitor->n = NULL;

    /* polling all sub-dirs now, child process should block here */
    if (monitors_poll()) {
      fprintf(stderr, "@monitor_init() @child: monitors_poll() fails\n");
      exit(1);
    }

    /* exit routine, not necessary */
    if (0 > close(rfd)) {
      perror("@monitors_init() @child: close rfd fails");
      return 1;
    }
    FD_CLR(rfd, &set);
    
    exit(0);
  }
}

/* I/O polling happens here (in child process) */
static int monitors_poll()
{
  monitor *temp_monitor;
  fd_set rset;
  int fd_count;
  int i = 0;
  int len;
  char buf[BUF_LEN];
  
  while (1) {
    rset = set;
    if ( 0 > (fd_count = select(maxfd+1, &rset, NULL, NULL, NULL))) {
      if (errno == EINTR) {
	perror("experiment success"); fflush(stderr);
	return 0;
      } else {
	perror("@monitors_poll(): select fails");
	return 1;
      }
    }

    /* if come from FIFO, which means monitors_cleanup() been called
     * and send 'exit' to child process, we read it and confirm */
    if (FD_ISSET(rfd, &rset)) {
      if (0 > (len = read(rfd, buf, BUF_LEN))) {
	perror("@monitors_poll(): read from rfd fails");
	return 1;
      }
      buf[len] = 0;
      
      if (0 == strncmp(buf, "exit", 4)) {
	for (temp_monitor = tail_monitor;
	     temp_monitor != NULL;
	     temp_monitor = temp_monitor->p)
	  monitor_disconnect(temp_monitor);
      }
      return 0;
    }
    
    /* if not come from FIFO, iterate every sub-dir in linked list
     * to find it */
    for (temp_monitor = tail_monitor; temp_monitor != NULL;
	 temp_monitor = temp_monitor->p) {
      if (FD_ISSET(temp_monitor->fd, &rset)) {
	if (0 > (len = read(temp_monitor->fd, buf, BUF_LEN))) {
	  perror("@monitors_poll(): read fails");
	  return 1;
	}
	i = 0;

	/* inotify handle routine */
	while (i < len) {
	  struct inotify_event *event = (struct inotify_event *)&buf[i];
	  if (event->len &&
	      (INCLUDE_HIDDEN ||
	       !(INCLUDE_HIDDEN ||*(event->name) == '.'))) {
	    /* DIR detected*/
	    if (event->mask & IN_ISDIR) {
	      printf("DIR %s was ", event->name);
	      
	      /* if CREATE detected, this new dir
	       * should be added to linked list */
	      if (event->mask & IN_CREATE) {
		printf("created ");
		if (monitor_connect_via_fpath(temp_monitor, event->name)) {
		  fprintf(stderr,
			  "@minitors_poll(): monitor_con_via_fname(%s, %s)",
			  temp_monitor->pathname,
			  event->name);
		  return 1;
		}

		//modified
	      } else if (event->mask & IN_MODIFY) {
		printf("modfied ");

		/* if DELETE detected, this departed dir
		 * should be removed from linked list */
	      } else if (event->mask & IN_DELETE) {
		printf("deleted ");
		if (monitor_disconnect_via_fpath(temp_monitor,
						event->name)) {
		  fprintf(stderr,
			  "@minitors_poll():" \
			  "monitor_discon_via_fname(%s, %s)",
			  temp_monitor->pathname,
			  event->name);
		  return 1;
		}

		/* MOVED_FROM detected, same as DELETE */
	      } else if (event->mask & IN_MOVED_FROM) {
		printf("moved from ");
		if (monitor_disconnect_via_fpath(temp_monitor,
						event->name)) {
		  fprintf(stderr,
			  "@minitors_poll():" \
			  "monitor_discon_via_fname(%s, %s)",
			  temp_monitor->pathname,
			  event->name);
		  return 1;
		}

		/* MOVED_TO detected, same as CREATE */
	      } else if (event->mask & IN_MOVED_TO) {
		printf("moved to ");
		if (monitor_connect_via_fpath(temp_monitor, event->name)) {
		  fprintf(stderr,
			  "@minitors_poll(): monitor_con_via_fname(%s, %s)",
			  temp_monitor->pathname,
			  event->name);
		  return 1;
		}
	      }

	      printf("under %s\n", temp_monitor->pathname);
	      fflush(stdout);
	      /* FILE detected*/
	    } else {
	      printf("FILE %s was ", event->name);
	      if (event->mask & IN_CREATE) {
		printf("created ");
	      } else if (event->mask & IN_MODIFY) {
		printf("modfied ");
	      } else if (event->mask & IN_DELETE) {
		printf("deleted ");
	      } else if (event->mask & IN_MOVED_FROM) {
		printf("moved from ");
	      } else if (event->mask & IN_MOVED_TO) {
		printf("moved to ");
	      }
	      
	      printf("under %s\n", temp_monitor->pathname);
	      fflush(stdout);
	    }
	  }
	  i += sizeof(struct inotify_event) + event->len;
	}  //end while(i < len)
      }  //end if(FD_ISSET)
    }  //end for(....
  }  //end while(1)
}  //end function



/* terminate child process, send 'exit' to child, then close
 * unnecessary fds */
int monitors_cleanup()
{
  if ( 4 != write(wfd, "exit", 4)) {
    perror("@monitors_cleanup(): write exit to wfd fails");
    return 1;
  }
  
  if (0 > close(wfd)) {
    perror("@monitors_cleanup(): close wfd fails");
    return 1;
  }
  
  if (0 > unlink(FIFO0)) {
    perror("@monitors_cleanup(): unlink FIFO0 fails");
    return 1;
  }
  if (0 > unlink(FIFO1)) {
    perror("@minitors_cleanup(): unlink FIFO1 fails");
    return 1;
  }
    
  /* if (0 > kill(pid, SIGINT)) { */
  /*   perror("@monitors_cleanup(): fail to kill SIGINT to child"); */
  /*   return 1; */
  /* } */
  /* printf("sigint sended, send to %d\n", pid); fflush(stdout); */
  return 0;
}

/* connect direcotry's full pathname with file(dir)'s name */
static int join_fname(const monitor *this_monitor,
		  const char *fname)
{
  char *ptr;
  
  if (!strncpy(fullpath, this_monitor->pathname,
	       strlen(this_monitor->pathname))) {
    fprintf(stderr,
	      "@monitor_con_via_fname: fail to strncpy %s to fullpath. %s\n",
	      this_monitor->pathname, strerror(errno));
    return 1;
  }

  ptr = fullpath + strlen(this_monitor->pathname);
  *ptr++ = '/';
  *ptr = 0;

  if (strncpy(ptr, fname,
		strlen(fname)) == NULL) {
      fprintf(stderr,
	      "@monitor_con_via_fname(): fail to strncpy %s to %s. %s\n",
	      fname, fullpath, strerror(errno));
      return 1;
    }
  *(ptr+strlen(fname)) = 0;

  return 0;
}

/* called when a directory is created or moved in at current
 * monitoring directory,
 * iteration deepin() should be used here because this
 * new directory may still contain sub-directory*/
static int monitor_connect_via_fpath(const monitor *this_monitor,
				   const char *fname)
{
  if (join_fname(this_monitor, fname)) {
    fprintf(stderr,
	    "@monitor_con_via_fpath(): join_name(%s, %s) failed\n",
	    this_monitor->pathname, fname);
    return 1;
  }

  if (nftw(fullpath, monitor_connect, 10, FTW_DEPTH) != 0) {
    perror("@monitor_connect_via_fpath(): nftw() failed");
    return 1;
  }
  tail_monitor->n = NULL;
  
  return 0;
}
  

/* create a linked list node, then add it to linked list */
static int monitor_connect(const char *path, const struct stat *sb,
			   int tflag, struct FTW *fb)
{

  // tflag doesn't work here, don't konw why
  if (!S_ISDIR(sb->st_mode))
    return 0;
  if (!INCLUDE_HIDDEN && (*(path + fb->base) == '.'))
    return 0;
  if (strncmp(FSS_DIR, path+fb->base, strlen(FSS_DIR) == 0))
    return 0;

  /* create part */
  monitor *temp_monitor;
  temp_monitor = (monitor*)calloc(1, sizeof(monitor));
  temp_monitor->pathname = (char*)calloc(strlen(path)+1,
					 sizeof(char));
  if (!strncpy(temp_monitor->pathname, path, strlen(path))) {
    fprintf(stderr,
	    "@monitor_connect(): "\
	    "fail to strncpy %s to temp_monitor->pathname. %s\n",
	    path, strerror(errno));
    return 1;
  }
  *(temp_monitor->pathname+strlen(path)) = 0;

  if (0 > (temp_monitor->fd = inotify_init())) {
    fprintf(stderr,
	    "@monitor_connect(): "\
	    "inotify_init() --%s-- fail. %s\n",
	    temp_monitor->pathname, strerror(errno));
    return 1;
  }

  temp_monitor->wd = inotify_add_watch(temp_monitor->fd,
				       temp_monitor->pathname,
				       mask);
  FD_SET(temp_monitor->fd, &set);
  maxfd = maxfd > temp_monitor->fd ? maxfd : temp_monitor->fd;
  

  /* connect part */
  temp_monitor->p = tail_monitor;
  
  /* cur->next = temp is omitted if tail_monitor is NULL
   * which means temp_monitor is head of this linked list
   */
  if (tail_monitor) 
    tail_monitor->n = temp_monitor;
  
  tail_monitor = temp_monitor;
  
  /* fprintf(stderr, "%s connected, fd=%d\n", tail_monitor->pathname, */
  /* 	 tail_monitor->fd); */

  return 0;
}
/* called when a directory is removed, we also remove all sub-dir
   under it via a trick on strncmp */
static int monitor_disconnect_via_fpath(const monitor *this_monitor,
				      const char *fname)
{
  //printf("in dink_disconnect_via_fpath\n"); fflush(stdout);
  monitor *temp_monitor;
  
  if (join_fname(this_monitor, fname)) {
    fprintf(stderr,
	    "@monitor_con_via_fpath(): join_name(%s, %s) fails.\n",
	    this_monitor->pathname, fname);
    return 1;
  }

  for (temp_monitor = tail_monitor; temp_monitor != NULL;
       temp_monitor = temp_monitor->p)
    /* compare with length of fullpath, so sub-dir will be removed
     * from linked list, too */
    if (0 == strncmp(temp_monitor->pathname, fullpath, strlen(fullpath)))
      monitor_disconnect(temp_monitor);

  return 0;
}

/* first disconnect this node from linked list,
 * then free it */
static int monitor_disconnect(monitor *this_monitor)
{
   //head and tail_monitor of list
  if (!this_monitor->n && !this_monitor->p)
    ;
  else if (!this_monitor->n) {  //tail of list
    this_monitor->p->n = NULL;
    tail_monitor = this_monitor->p;
  } else if (!this_monitor->p) { //head of list
    this_monitor->n->p = NULL;
  } else {
    this_monitor->p->n = this_monitor->n;
    this_monitor->n->p = this_monitor->p;
  }

  if (0 > close(this_monitor->fd)) {
    fprintf(stderr,
	    "@monitor_disconnect(): close(fd) %s fails.\n",
	    this_monitor->pathname);
    return 1;
  }
  FD_CLR(this_monitor->fd, &set);
  /* fprintf(stdout, "%s disconected\n", this_monitor->pathname); */
  /* fflush(stdout); */
  free(this_monitor->pathname);
  free(this_monitor);

  return 0;
}

static void sig_child(int signo)
{
  int stat;

  if (0 > wait(&stat))
    perror("@sig_child(): wait fails.");

  //printf("waited\n"); fflush(stdout);

  return ;
}
