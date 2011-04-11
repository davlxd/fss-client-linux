/*
 * Log on functions 
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

#include "log.h"

void fsslog(const char *msg, ...)
{
  va_list ap;
  va_start(ap, msg);
  logit(msg, ap);
  va_end(ap);
}

static void logit(const char *fmt, va_list ap)
{
  char buf[LOG_RECORD_MAX_LEN];
  const time_t cur_time = time(NULL);
  char *timestr;

  vsnprintf(buf, LOG_RECORD_MAX_LEN, fmt, ap);

  if (!log_fp){
    mode_t old_mask = umask(022);
    log_fp = fopen(log_fname, "a+");
    umask(old_mask);
    if (!log_fp)
      syslog(LOG_ERR, "fss-client: cannot open log file");
  }
  if (log_fp){
    timestr = ctime(&cur_time);
    timestr[strlen(timestr)-1] = 0;
    fprintf(log_fp, "%s\t%d\t%s\n", timestr, (int)getpid(), buf);
    fflush(log_fp);
    if (0 != fclose(log_fp))
      syslog(LOG_ERR, "fss-client: cannot close log file");
    log_fp = NULL;
  } else
    syslog(LOG_ERR, "fss-client: %s", buf);
}
