/*
 * core, client
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

#include "protocol.h"

static int status;
static int sockfd;
static int monifd;

// monitor lock 
static int lock;

static char rela_name[MAX_PATH_LEN];
static time_t mtime;
static off_t req_sz;

static int handle_monifd();
static int handle_sockfd();
static int status_WAIT_SHA1_FSS_INFO();
static int status_WAIT_SHA1_FSS();
static int status_WAIT_ENTRY_INFO();
static int status_WAIT_FILE();
static int status_WAIT_MSG_SER_RECEIVED();
static int status_WAIT_MSG_SER_REQ_FILE();
static int status_WAIT_MSG_SER_REQ_DEL_IDX();
static int receive_line(int, char*, int);
static int set_fileinfo(char*);
static int analyse_sha1();
static int download_sync();

int client_polling(int moni_fd, int sock_fd)
{
  fd_set rset;
  int maxfd;

  sockfd = sock_fd;
  monifd = moni_fd;

  if (remove_diffs()) {
    fprintf(stderr, "@client_polling(): remove_diffs() failed\n");
    return 1;
  }
  status = WAIT_SHA1_FSS_INFO;
  lock = 1;

  while(1) {
    
    FD_SET(monifd, &rset);
    FD_SET(sockfd, &rset);
    maxfd = monifd > sockfd ? monifd : sockfd;

    printf("\n>>>> block at select() now ...\n");

    if (select(maxfd + 1, &rset, NULL, NULL, NULL) < 0) {
      perror("@client_polling(): select() failed");
      return 1;
    }
    printf(">>>> select() detect something..\n");
    if (FD_ISSET(monifd, &rset)) {
      if (handle_monifd(monifd, sockfd)) {
	fprintf(stderr, "@client_polling(): handle_monifd() failed");
	return 1;
      }
    }

    if (FD_ISSET(sockfd, &rset)) {
      if (handle_sockfd()) {
	fprintf(stderr, "@client_polling(): handle_sockfd() failed");
	return 1;
      }
    }
  }

  return 0;
}

static int handle_monifd()
{
  printf(">>>> IN handle_monifd()\n");
  int n;
  int rv, rvv;
  
  if (lock)
    return 0;
  

  /* following code provide verbose output for testing ... */
  char buffer[BUF_LEN];
  if ((n = read(monifd, buffer, BUF_LEN)) < 0) {
    perror("@handle_monifd(): read() failed");
    return 1;
  }
  buffer[n] = 0;
  printf(">>>> INOTIFY detected: %s\n", buffer);
  /* end */

  if (update_files()) {
    fprintf(stderr, "@handle_monifd(): update_files() failed\n");
    return 1;
  }

  printf(">>>> generateing files ...\n");
  if ((rv = generate_diffs()) == 1) {
    fprintf(stderr, "@handle_sockfd(): generate_diffs() failed\n");
    return 1;

  } else if (rv == DIFF_BOTH_UNIQ || rv == DIFF_LOCAL_UNIQ) {
    // lock on
    lock = 1;
    printf(">>>> lock set\n");
    if ((rvv = send_entryinfo_or_reqsha1info(sockfd, 1, FILE_INFO, DIR_INFO,
					     CLI_REQ_SHA1_FSS_INFO)) == 1) {
      fprintf(stderr,
	      "@handle_monifd(): send_entryinfo_or_reqsha1info() failed\n");
      return 1;
	    
    } else if (rvv == PREFIX0_SENT)
      status = WAIT_MSG_SER_REQ_FILE;

    else if (rvv == PREFIX1_SENT)
      status = WAIT_MSG_SER_RECEIVED;

    else if (rvv == PREFIX2_SENT)
      status = WAIT_SHA1_FSS_INFO;

  } else if (rv == DIFF_REMOTE_UNIQ) {
    lock = 1;
    printf(">>>> lock set\n");
    if (send_del_index_info(sockfd, DEL_IDX_INFO)) {
      fprintf(stderr, "@handle_monifd(): send_del_index_info() failed\n");
      return 1;
    }
    status = WAIT_MSG_SER_REQ_DEL_IDX;
  }
    

  return 0;
}


static int handle_sockfd()
{
  printf(">>>> socket readable\n");
  char buf[MAX_PATH_LEN];
  memset(buf, 0, MAX_PATH_LEN);

  switch (status) {
    
  case WAIT_SHA1_FSS_INFO:
    if (status_WAIT_SHA1_FSS_INFO(sockfd)) {
      fprintf(stderr,
	      "@handle_sockfd(): status_WAIT_SHA1_FSS_INFO() failed\n");
      return 1;
    }
    break;


  case WAIT_SHA1_FSS:
    if (status_WAIT_SHA1_FSS(sockfd)) {
      fprintf(stderr,
	      "@handle_sockfd(): status_WAIT_SHA1_FSS() failed\n");
      return 1;
    }
    break;


  case WAIT_ENTRY_INFO:
    if (status_WAIT_ENTRY_INFO(sockfd)) {
      fprintf(stderr,
	      "@handle_sockfd(): status_WAIT_ENTRY_INFO() failed\n");
      return 1;
    }
    break;


  case WAIT_FILE:
    if (status_WAIT_FILE(sockfd)) {
      fprintf(stderr,
	      "@handle_sockfd(): status_WAIT_FILE() failed\n");
      return 1;
    }
    break;


  case WAIT_MSG_SER_REQ_FILE:
    if (status_WAIT_MSG_SER_REQ_FILE(sockfd)) {
      fprintf(stderr,
	      "@handle_sockfd(): status_WAIT_MSG_SER_REQ_FILE() failed\n");
      return 1;
    }
    break;


  case WAIT_MSG_SER_RECEIVED:
    if (status_WAIT_MSG_SER_RECEIVED(sockfd)) {
      fprintf(stderr,
	      "@handle_sockfd(): status_WAIT_SER_RECEIVED() failed\n");
      return 1;
    }
    break;


  case WAIT_MSG_SER_REQ_DEL_IDX:
    if (status_WAIT_MSG_SER_REQ_DEL_IDX()) {
      fprintf(stderr,
	      "@handle_sockfd(): "\
	      "status_WAIT_MSG_SER_REQ_DEL_IDX() failed\n");
      return 1;
    }
    break;


  default:
    fprintf(stderr, "@handle_sockfd(): unknow status %d captured\n",
	    status);
    return 1;
  }
    


  return 0;
}


static int status_WAIT_SHA1_FSS_INFO()
{
  printf(">>>> ---> WAIT_SHA1_FSS_INFO\n");
  char buf[MAX_PATH_LEN];
  
  if (receive_line(sockfd, buf, MAX_PATH_LEN)) {
    fprintf(stderr,
	    "@status_WAIT_SHA1_FSS_INFO(): received_line() failed\n");
    return 1;
  }

  if (strncmp(buf, SHA1_FSS_INFO, strlen(SHA1_FSS_INFO)) == 0) {
    lock = 1;
    printf(">>>> lock is on\n");

    if (set_fileinfo(buf+strlen(SHA1_FSS_INFO))) {
      fprintf(stderr,
	      "@status_WAIT_SHA1_FSS_INFO(): set_fileinfo() failed\n");
      return 1;
    }

    if (send_msg(sockfd, CLI_REQ_SHA1_FSS)) {
      fprintf(stderr, "@status_WAIT_SHA1_FSS_INFO(): send_msg() failed\n");
      return 1;
    }
    
    // if server gonna send a 0 byte file, it won't acturally send
    // it, client will stuck at read()
    // so directly invoke receive_sha_fss() routine (receive_file can
    // handle 0 byte receivation )
    if (req_sz == 0) {
      if (status_WAIT_SHA1_FSS()) {
	fprintf(stderr,
		"@status_WAIT_SHA1_FSS_INFO(), " \
		"status_WAIT_SHA1_FSS() failed\n");
	return 1;
      }
    } else {
      status = WAIT_SHA1_FSS;
      printf(">>>> status set to ----> WAIT_SHA1_FSS\n");
    }

  } else {
    printf("WARNING: current status WAIT_SHA1_FSS_INFO"\
	   " received invalid message: %s\n", buf);
    return 0;
  }

  return 0;
}

static int status_WAIT_SHA1_FSS()
{
  printf(">>>> ---> WAIT_SHA1_FSS \n");
  
  if (receive_sha1_file(sockfd, req_sz)) {
    fprintf(stderr, "@status_WAIT_SHA1_FSS(): receive_sha1_file() failed\n");
    return 1;
  }
  printf(">>>> sha1_file received, updating local sha1.fss ...\n");

  if (update_files()) {
    fprintf(stderr, "@status_WAIT_SHA1_FSS(): update_files() failed\n");
    return 1;
  }

  if (analyse_sha1()) {
    fprintf(stderr, "@status_WAIT_SHA1_FSS(): analyse_sha1() failed\n");
    return 1;
  }

  return 0;
}



static int analyse_sha1()
{
  int rv, rvv;

   if ((rv = generate_diffs()) == 1) {
      fprintf(stderr, "@analyse_sha1(): generate_diffs() failed\n");
      return 1;
   }
   // local's sha1.fss is newer
   if (mtime <= sha1_fss_mtime()) {
     if (rv == DIFF_BOTH_UNIQ || rv == DIFF_LOCAL_UNIQ) {
       if ((rvv = send_entryinfo_or_reqsha1info(sockfd, 1, FILE_INFO,
						DIR_INFO, 
						CLI_REQ_SHA1_FSS_INFO))
	   == 1) {
	 fprintf(stderr,
		 "@analyse_sha1(): send_entryinfo_or_done() failed\n");
	 return 1;
	 
       } else if (rvv == PREFIX0_SENT) 
	 status = WAIT_MSG_SER_REQ_FILE;

       else if (rvv == PREFIX1_SENT) 
	 status = WAIT_MSG_SER_RECEIVED;

       else if (rvv == PREFIX2_SENT)
	 status = WAIT_SHA1_FSS_INFO;

     } else if (rv == DIFF_IDENTICAL) {
       if (mtime == (time_t)1) {
	 if (send_msg(sockfd, FIN)) {
	   fprintf(stderr, "@analyse_sha1(): send_msg() failed\n");
	   return 1;
	 }
       } else {
	 if (send_msg(sockfd, DONE)) {
	   fprintf(stderr, "@analyse_sha1(): send_msg() failed\n");
	   return 1;
	 }
       }
       status = WAIT_SHA1_FSS_INFO;
       printf(">>>> status set to ------> WAIT_SHA1_FSS_INFO\n");
       
       lock = 0;
       printf(">>>> lock unset\n");
     } else if (rv == DIFF_REMOTE_UNIQ) {
       if (send_del_index_info(sockfd, DEL_IDX_INFO)) {
	 fprintf(stderr,
		 "@analyse_sha1(): send_del_index_info() failed\n");
	 return 1;
       }
       status = WAIT_MSG_SER_REQ_DEL_IDX;
     }
     // remote.sha1.fss is newer
   } else {
     if (rv == DIFF_BOTH_UNIQ || rv == DIFF_REMOTE_UNIQ) {
       if ((rvv = send_linenum_or_done(sockfd, 1, LINE_NUM, DONE)) == 1) {
	 fprintf(stderr,
		 "@analyse_sha1(): send_linenum_or_done() failed\n");
	 return 1;
       }
       status = WAIT_ENTRY_INFO;
       printf(">>>> status set to ----> WAIT_ENTRY_INFO\n");
     } else if (rv == DIFF_IDENTICAL) {
       if (send_msg(sockfd, DONE)) {
	 fprintf(stderr, "@analyse_sha1(): send_msg() failed\n");
	 return 1;
       }
       lock = 0;
       printf(">>>> lock unset\n");

       status = WAIT_SHA1_FSS_INFO;
       printf(">>>> status set to ----> WAIT_SHA1_FSS_INFO\n");
     } else if (rv == DIFF_LOCAL_UNIQ) {
       if (remove_files()) {
	 fprintf(stderr, "@analyse_sha1(): remove_files() failed\n");
	 return 1;
       }
       if (update_files()) {
	 fprintf(stderr, "@analyse_sha1(): update_files() failed\n");
	 return 1;
       }
       if (send_msg(sockfd, DONE)) {
	 fprintf(stderr, "@analyse_sha1(): send_msg() failed\n");
	 return 1;
       }
       lock = 0;
       printf(">>>> lock unset\n");

       status = WAIT_SHA1_FSS_INFO;
       printf(">>>> status set to ----> WAIT_SHA1_FSS_INFO\n");
     }
   }

   return 0;
}




static int status_WAIT_ENTRY_INFO()
{
  printf(">>>> >>>> WAIT_ENTRY_INFO\n");
  char buf[MAX_PATH_LEN];
  
  if (receive_line(sockfd, buf, MAX_PATH_LEN)) {
    fprintf(stderr, "@status_WAIT_FILE_INFO(): received_line() failed\n");
    return 1;
  }
  printf(">>>> received %s\n", buf);

  if (strncmp(buf, DIR_INFO, strlen(DIR_INFO)) == 0) {
    char *token;
    token = strtok(buf+strlen(DIR_INFO), "\n");
    if (create_dir(token)) {
      fprintf(stderr, "@status_WAIT_ENTRY_INFO(): create_dir() failed\n");
      return 1;
    }

    if (download_sync()) {
      fprintf(stderr, "@status_WAIT_ENTRY_INFO(): download_sync() failed\n");
      return 1;
    }

  } else if (strncmp(buf, FILE_INFO, strlen(FILE_INFO)) == 0) {
    if (set_fileinfo(buf+strlen(FILE_INFO))) {
      fprintf(stderr, "@status_WAIT_FILE_INFO(): set_fileinfo() failed\n");
      return 1;
    }
    if (send_msg(sockfd, CLI_REQ_FILE)) {
      fprintf(stderr, "@status_WAIT_FILE_INFO(): send_msg() failed\n");
      return 1;
    }

    if (req_sz == 0 ) {
      if (status_WAIT_FILE()) {
	fprintf(stderr,
		"@status_WAIT_FILE_INFO(): status_WAIT_FILE() failed\n");
	return 1;
      }
    } else 
      status = WAIT_FILE;

    printf(">>>> CLI_REQ_FILE sent, status sent to ---> WAIT_FILE\n");

  } else {
    printf("WARNING: current status WAIT_SHA1_FSS_INFO"\
	   " received invalid message: %s\n", buf);
    return 0;
  }

  return 0;
}



static int status_WAIT_FILE()
{
  printf(">>>> ---> WAIT_FILE\n");
  
  if (receive_common_file(sockfd, rela_name, req_sz)) {
    fprintf(stderr,
	    "@status_WAIT_FILE(): receive_common_file failed\n");
    return 1;
  }

  if (download_sync()) {
    fprintf(stderr, "@status_WAIT_FILE(): download_sync() failed\n");
    return 1;
  }

  return 0;
}


static int download_sync()
{
  int rv, rvv, rvvv;

  if ((rv = send_linenum_or_done(sockfd, 0, LINE_NUM, DONE)) == 1) {
    fprintf(stderr,
	    "@status_WAIT_FILE(): send_linenum_or_done() failed\n");
    return 1;

  } else if (rv == PREFIX0_SENT) {
    status = WAIT_ENTRY_INFO;
    printf(">>>> linenum sent, status sent to --->  WAIT_FILE_INFO\n");

    // sent DONE
  } else if (rv == PREFIX1_SENT) {

    if (update_files()) {
      fprintf(stderr, "@status_WAIT_FILE(): update_files() failed\n");
      return 1;
    }

    /* analyse again, just in case user change files during download
     * sync process*/
    if ((rvv = generate_diffs()) == 1) {
      fprintf(stderr, "@status_WAIT_FILE(): generate_diffs() failed\n");
      return 1;

    } else if (rvv == DIFF_BOTH_UNIQ || rvv == DIFF_REMOTE_UNIQ) {

      if ((rvvv = send_entryinfo_or_reqsha1info(sockfd, 1, FILE_INFO,
						DIR_INFO,
						CLI_REQ_SHA1_FSS_INFO))
	  == 1) {
	fprintf(stderr,
		"@status_WAIT_FILE(): " \
		"send_entryinfo_or_reqsha1info() failed\n");
	return 1;
	    
      } else if (rvvv == PREFIX0_SENT) 
	status = WAIT_MSG_SER_REQ_FILE;

      else if (rvvv == PREFIX1_SENT)
	status = WAIT_MSG_SER_RECEIVED;

      else if (rvvv == PREFIX2_SENT)
	status = WAIT_SHA1_FSS_INFO;
      
      /* Attension, if user add some files during download sync process
       * program still goes to here, hereby being deleted */
    } else if (rvv == DIFF_LOCAL_UNIQ || rvv == DIFF_IDENTICAL) {

      if (rvv == DIFF_LOCAL_UNIQ) {
	if (remove_files()) {
	  fprintf(stderr, "@status_WAIT_FILE(): remove_files() failed\n");
	  return 1;
	}
	printf(">>>> obselot files removed\n");
      }
      
      /* if (remove_diffs()) { */
      /* fprintf(stderr, "@status_WAIT_FILE(): remove_diffs() failed\n"); */
      /* return 1; */
      /* } */
      /* printf(">>>> diff.* files removed\n"); */

      lock = 0;
      printf(">>>> lock unset\n");

      status = WAIT_SHA1_FSS_INFO;
      printf(">>>> status sent to ---> WAIT_SHA1_FSS_INFO\n");
    }
    
  }

  return 0;
}


static int status_WAIT_MSG_SER_REQ_FILE()
{
  printf(">>>> ---> WAIT_MSG_SER_REQ_FILE\n");
  char buf[MAX_PATH_LEN];
  
  if (receive_line(sockfd, buf, MAX_PATH_LEN)) {
    fprintf(stderr,
	    "@status_WAIT_MSG_SER_REQ_FILE(): received_line() failed\n");
    return 1;
  }
  printf(">>>> received %s\n", buf);
  if (strncmp(buf, SER_REQ_FILE, strlen(SER_REQ_FILE)) == 0) {

    // via files.c/h's internal linenum
    if (send_file_via_linenum(sockfd)) {
      fprintf(stderr,
	      "@status_WIAT_MSG_SER_REQ_FILE(): set_fileinfo() failed\n");
      return 1;
    }
    status = WAIT_MSG_SER_RECEIVED;
    printf(">>>> file via linenum sent, status set to ---> WAIT_MSG_SER_RECEIVED\n");

  } else {
    printf("WARNING: current status WAIT_MSG_SER_REQ_FILE"\
	   " received invalid message: %s\n", buf);
    return 0;
  }
  return 0;
}

static int status_WAIT_MSG_SER_RECEIVED()
{
  printf(">>>> ---> WAIT_MSG_SER_RECEIVED\n");
  int rv;
  char buf[MAX_PATH_LEN];
  
  if (receive_line(sockfd, buf, MAX_PATH_LEN)) {
    fprintf(stderr,
	    "@status_WAIT_MSG_SER_RECEIVED(): received_line() failed\n");
    return 1;
  }
  printf(">>>> received %s\n", buf);
  if (strncmp(buf, SER_RECEIVED, strlen(SER_RECEIVED)) == 0) {
    if ((rv = send_entryinfo_or_reqsha1info(sockfd, 0, FILE_INFO, DIR_INFO,
					    CLI_REQ_SHA1_FSS_INFO)) == 1) {
      fprintf(stderr,
	      "@status_WAIT_MSG_SER_RECEIVED(): "\
	      "send_entryinfo_or_reqsha1info() failed\n");
      return 1;
	    
    } else if (rv == PREFIX0_SENT) {
      status = WAIT_MSG_SER_REQ_FILE;
      printf(">>>> fileinfo sent, status set to ---> WAIT_MSG_SER_REQ_FILE\n");
      
    } else if (rv == PREFIX1_SENT) {
      status = WAIT_MSG_SER_RECEIVED;
      printf(">>>> done sent, status set to ---> WAIT_SHA1_FSS_INFO\n");

    } else if (rv == PREFIX2_SENT) {
      status = WAIT_SHA1_FSS_INFO;
    }
  }

  return 0;
}

static int status_WAIT_MSG_SER_REQ_DEL_IDX()
{
  printf(">>>> ---> WAIT_MSG_SER_REQ_DEL_IDX\n");
  char buf[MAX_PATH_LEN];
  
  if (receive_line(sockfd, buf, MAX_PATH_LEN)) {
    fprintf(stderr,
	    "@status_WAIT_MSG_SER_REQ_DEL_IDX(): received_line() failed\n");
    return 1;
  }
  printf(">>>> received %s\n", buf);
  if (strncmp(buf, SER_REQ_DEL_IDX, strlen(SER_REQ_DEL_IDX)) == 0) {

    if (send_del_index(sockfd)) {
      fprintf(stderr,
	      "@status_WIAT_MSG_SER_REQ_DEL_IDX(): "\
	      "send_del_index_file() failed\n");
      return 1;
    }

    /* lock = 0; */
    /* printf(">>>> lock unset\n"); */

    status = WAIT_SHA1_FSS_INFO;
    printf(">>>> del_index sent, status set to ---> WAIT_SHA1_FSS_INFO\n");

  } else {
    printf("WARNING: current status WAIT_MSG_SER_REQ_FILE"\
	   " received invalid message: %s\n", buf);
    return 0;
  }

  return 0;
}



static int receive_line(int sockfd, char *text, int len)
{
  int n;

  //TODO: A while() should be place here, within MAX_PATH_LEN
  if ((n = read(sockfd, text, len)) < 0) {
    perror("@receive_line(): read failed");
    return 1;
  }
  text[n] = 0;
  printf(">>>> receive_line received %s\n", text);
  if (n == 0) {
    fprintf(stderr, "@receive_line(): read 0, server may crash\n");
    return 1;
  }

  return 0;
}

static int set_fileinfo(char *buf)
{
  printf(">>>> In set_fileinfo string %s",buf); 

  char *token;
  token = strtok(buf, "\n");
  if(strncpy(rela_name, token, strlen(token)) == NULL) {
    perror("@set_fileinfo(): strncpy failed");
    return 1;
  }
  rela_name[strlen(token)] = 0;
  token = strtok(NULL, "\n");
  
  /* Attention:
   * I assign off_t and time_t to long */
  errno = 0;
  if (((mtime = strtol(token, NULL, 10)) == 0) && errno != 0) {
    perror("@set_fileinfo(): strtol size failed");
    return 1;
  }
  token = strtok(NULL, "\n");

  errno = 0;
  if (((req_sz = strtol(token, NULL, 10)) == 0) && errno != 0) {
    perror("@set_fileinfo(): strtol size failed");
    return 1;
  }

  printf("   set\n"); 
  return 0;
}
