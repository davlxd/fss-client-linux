/*
 * Linearly diff match algorithm
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
#include "diff.h"

int diff(const char *fin0, const char *fin1,
	 const char *fout0, const char *fout1, const char *fout2)
{
  long i, j;
  long file_in_0_line_num, file_in_1_line_num;
  char buf0[MAX_LINE_LEN];
  char buf1[MAX_LINE_LEN];

  if (open_them(fin0, fin1, fout0, fout1, fout2)) {
    fprintf(stderr, "@diff(): open_them() failed\n");
    return 1;
  }

  if (get_line_num(&file_in_0_line_num, file_in_0)) {
    fprintf(stderr, "@diff(): get_line_num() failed");
    return 1;
  }
  if (get_line_num(&file_in_1_line_num, file_in_1)) {
    fprintf(stderr, "@diff(): get_line_num() failed");
    return 1;
  }

  int *flag = (int*)malloc(file_in_1_line_num * sizeof(int));
  for (j = 0; j < file_in_1_line_num; j++)
    flag[j] = 0;
  
  rewind(file_in_0);
  for (i = 0; i < file_in_0_line_num; i++) {
    fgets(buf0, MAX_LINE_LEN, file_in_0);
    //printf(">>>> diff: Now --%s--%ld-- from file_in_0 read\n", buf0, i+1);
    rewind(file_in_1);
    for (j = 0; j < file_in_1_line_num; j++) {
      fgets(buf1, MAX_LINE_LEN, file_in_1);
      // printf(">>>> diff: Now --%s--%ld-- from file_in_1 read\n", buf1, j+1);
      if (flag[j]) {
	//printf("Setted\n");
	continue;
      }
      if (strncmp(buf0, buf1, 40) == 0) {
	//printf("Setting\n");
	flag[j] = 1;
	break;
      }
    }
    if (j == file_in_1_line_num) {
      //printf(">>> diff: Line --%s--%ld-- from file_in_0 unique\n",
      //buf0, i+1); 
      if (write_line_num(i+1, file_out_0)) {
	fprintf(stderr, "@diff(): write_line_num() failed");
	return 1;
      }
    }
  }

  for (j = 0; j < file_in_1_line_num; j++) {
    if (!flag[j]) {
      if (write_line_num(j+1, file_out_1)) {
	fprintf(stderr, "@diff(): write_line_num() failed");
	return 1;
      }
    } else {
      if (write_line_num(j+1, file_out_2)) {
	fprintf(stderr, "@diff(): write_line_num() failed");
	return 1;
      }
    }
  }
      

  free(flag);
  
  if (close_them()) {
    fprintf(stderr, "@diff(): close_them() failed\n");
    return 1;
  }
  return 0;
}


static int open_them(const char *fin0, const char *fin1,
		     const char *fout0, const char *fout1,
		     const char *fout2)
{
  
  if ((file_in_0 = fopen(fin0, "rb")) == NULL) {
    perror("@open_them(): fopen fin0 failed");
    return 1;
  }

  if ((file_in_1 = fopen(fin1, "rb")) == NULL) {
    perror("@open_them(): fopen fin1 failed");
    return 1;
  }

  if (fout0 != NULL) {
    if ((file_out_0 = fopen(fout0, "wb")) == NULL) {
      perror("@open_them(): fopen fout0 failed");
      return 1;
    }
  } else
    file_out_0 = NULL;

  if (fout1 != NULL) {
    if ((file_out_1 = fopen(fout1, "wb")) == NULL) {
      perror("@open_them(): fopen fout1 failed");
      return 1;
    }
  } else
    file_out_1 = NULL;

  if (fout2 != NULL) {
    if ((file_out_2 = fopen(fout2, "wb")) == NULL) {
      perror("@open_them(): fopen fou20 failed");
      return 1;
    }
  } else
    file_out_2 = NULL;

  return 0;
}


static int close_them()
{
  if (file_in_0 != NULL) {
    if (EOF == fclose(file_in_0)) {
      perror("@close_them(): fclose(file_in_0) failed");
      return 1;
    }
    file_in_0 = NULL;
  }

  if (file_in_1 != NULL) {
    if (EOF == fclose(file_in_1)) {
      perror("@close_them(): fclose(file_in_1) failed");
      return 1;
    }
    file_in_1 = NULL;
  }

  if (file_out_0 != NULL) {
    if (EOF == fclose(file_out_0)) {
      perror("@close_them(): fclose(file_out_0) failed");
      return 1;
    }
    file_out_0 = NULL;
  }

  if (file_out_1 != NULL) {
    if (EOF == fclose(file_out_1)) {
      perror("@close_them(): fclose(file_out_1) failed");
      return 1;
    }
    file_out_1 = NULL;
  }

  if (file_out_2 != NULL) {
    if (EOF == fclose(file_out_2)) {
      perror("@close_them(): fclose(file_out_2) failed");
      return 1;
    }
    file_out_2 = NULL;
  }

  return 0;
}


/* return value:
 *    -1     ->  error
 *     0     ->  file is NULL
 *    int>0  ->  line num
 */
static int get_line_num(long *num, FILE *file)
{
  off_t loc;
  int c;
  
  if (file == NULL)
    return 0;

  loc = ftello(file);
  rewind(file);

  *num = 0;
  while((c = getc(file)) != EOF) 
    if (c == '\n')
      (*num)++;
  if (ferror(file)) {
    perror("@get_line_num(): getc() failed");
    return 1;
  }
 
  if (fseeko(file, loc, SEEK_SET) != 0) {
    perror("@get_line_num(): fseeko failed");
    return 1;
  }

  return 0;
}

static int write_line_num(long num, FILE *file_out)
{
  if (file_out == NULL)
    return 0;
  else
    if (fprintf(file_out, "%ld\n", num) < 0) {
      perror("@write_line_num(): fprintf line num failed");
      return 1;
    }
   
   
  return 0;
}
  
