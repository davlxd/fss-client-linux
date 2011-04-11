/*
 * maintain .fss file in monitored direcotory, header file
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

#ifndef _FILES_H_
#define _FILES_H_

#define _XOPEN_SOURCE 500

#include "fss.h"
#include "diff.h"
#include "wrap-sha1.h"

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <ftw.h>

extern int errno;

#ifndef MAX_PATH_LEN
#define MAX_PATH_LEN 1024
#endif


#define FNAME_FSS "fname.fss"

#define SHA1_FSS "sha1.fss"
#define TEMP_SHA1_FSS "temp.sha1.fss"

/* remote.sha1.fss is ONLY server's sha.fss @ client */
#define REMOTE_SHA1_FSS "remote.sha1.fss"

/* remote.sha1.fss's unique sha1 record line_number in remote.sha1.fss */
#define DIFF_REMOTE_INDEX "diff.remote.index.fss"

/* local's sha1.fss's unique sha1 record line_number in sha1.fss */
#define DIFF_LOCAL_INDEX "diff.local.index.fss"

#define DEL_INDEX "del.index.fss"

#ifndef BUF_LEN
#define BUF_LEN 4096
#endif

/* rootpath do not end with '/' */
static char rootpath[MAX_PATH_LEN];

static long linenum_to_send;
static off_t size_to_send;

/* these 2 global via called by call back function write_in() */
static FILE *fname_fss;
static FILE *temp_sha1_fss;

// client
static FILE *diff_remote_index;
static FILE *diff_local_index;

int set_rootpath(const char *root_path);
int update_files();
int generate_diffs();

/* send.... */
int send_del_index_info(int sockfd, const char *prefix);

int send_del_index(int sockfd);
int send_file_via_linenum(int sockfd);
int send_file(int sockfd, const char *relaname);

int send_entryinfo_via_linenum(int sockfd, long linenum,
			       const char *prefix0, const char *prefix1);
int send_entryinfo(int sockfd, const char *fname,
		   const char *prefix0,  const char *prefix1);

int send_msg(int sockfd, const char *msg);


/* TODO:
 *     this 3 functions are a little bit noisy, should be rewriete */
//client
int send_linenum_or_done(int sockfd, int ifinit,
			 const char *prefix0, const char *prefix1);

int send_entryinfo_or_reqsha1info(int sockfd, int ifinit,
				  const char *prefix0,
				  const char *prefix1, const char *prefix2);

/* receive... */
int receive_sha1_file(int sockfd, off_t sz);
int receive_common_file(int sockfd, const char *rela_fname, off_t sz);
int receive_file(int sockfd, const char *relaname, off_t size);

int create_dir(const char *relafname);
int remove_dir(const char *fname);
static int fn(const char *fname, const struct stat *sb, int flag,
	      struct FTW *fb);

time_t sha1_fss_mtime();
int remove_diffs();
int remove_files();



/* the following funcions do explict path connecions
 * assume fpath is big enough */

static int get_fss_dir(char *fpath);
static int get_xxx(char *fpath, const char *name);
static int get_fname_fss(char *fpath);
static int get_sha1_fss(char *fpath);
static int get_temp_sha1_fss(char * fpath);
static int get_remote_sha1_fss(char * fpath);
static int get_diff_remote_index(char * fpath);
static int get_diff_local_index(char * fpath);
static int get_del_index(char * fpath);


// make sure path0 is large enough
static int connect_path(char *path0, const char *path1);

// without any check, make sure it is called after connect_path()
static int disconnect_path(char *path0, const char *path1);

static int create_fss_dir(const char *path);
static int write_in(const char *text, const struct stat *ptr,
		    int flag, struct FTW *fb);
static int get_line(const char *fname, long linenum,
		    char *buffer, int maxlen);

/* rela_path do not start with '/' */
static int get_rela_path(const char *fullpath, char *rela_path);

#endif
