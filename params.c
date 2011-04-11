/*
 * Read fss.conf file
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

#include "params.h"

/*
 * k is key, pass in
 * v is value-result argument
 */
int get_param_value(const char *k, char *v)
{
  if (open_file(&conf_fp)){
    //fsslog("params.c get_param_value(): fail to open conf_fp");
    fprintf(stderr, "@get_param_value(): fail to open_file(&conf_fp)\n");
    return 1;
  }

  if (parse(conf_fp, k, v)){
    //fsslog("params.c get_param_value(): fail to parse()");
    fprintf(stderr, "@get_param_value(): fail to parse()\n");
    return 1;
  }

  if (0 != fclose(conf_fp)){
    //fsslog("params.c get_param_value(): fail to close conf_fp");
    fprintf(stderr, "@get_param_value(): fail to fclose(conf_fp)\n");
    return 1;
  }
  conf_fp = NULL;

  return 0;
}

static int open_file(FILE **file)
{
  if (!*file){
    mode_t old_mask = umask(440);
    *file = fopen(conf_fname, "r");
    umask(old_mask);
    if (!*file) {
      //fsslog("params.c open_file(): fail to open config file, %s",
      //strerror(errno));
      perror("@open_file(): fail to open confi file");
      return 1;
    }
  }
  return 0;
}

static int skip_space(FILE *file)
{
  int c;
  for (c=getc(file); isspace(c)&&('\n'!=c); c=getc(file))
    ;
  return c;
}

static int skip_comment(FILE *file)
{
  int c;
  for (c=getc(file); ('\n'!=c)&&(EOF!=c)&&(c>0); c=getc(file))
    ;
  return c;
}

static int parse(FILE *file, const char *k, char *v)
{
  char c;
  int rv;
  int flag = 0;
  c = skip_space(file);

  while ( (EOF!=c) && (c>0) && !flag)
    {
      switch (c)
	{
	case '\n':
	  c = skip_space(file);
	  break;
	case ';':
	case '#':
	  c = skip_comment(file);
	  break;
	default:
	  if (1 == (rv = parse_param(file, c, k, v))){
	    //fsslog("params.c parse(): fail to parse_param()");
	    fprintf(stderr, "@parse(): fail to parse_param()\n");
	    return 0;
	  } else if (!rv) 
	    flag = 1;
	  
	  c = skip_space(file);
	  break;
	}
    }
  return 0;
}

// return 0 -> key found
// return 1 -> error
// return 2 -> key not found 
static int parse_param(FILE *file, char c, const char *k, char *v)
{

  int i = 0;  //current location of param or value
  int end = 0;  //end location of param or value
  int value_ptr = 0;  //start location of value in cur line

  char param[PARAM_LEN];
  char value[VALUE_LEN];

  while (!value_ptr)
    {
      switch (c)
	{
	case '=':
	  if (end == 0) {
	    //fsslog("params.c parse_param(): fail to parse conf file, "
	    // "empty param name");
	    fprintf(stderr, "@parse_param(): fail to parse conf file, "\
		    "empty param name\n");
	      return 1;
	    }
	  param[end] = 0;
	  value_ptr = 1;
	  break;

	case '\n':
	  //fsslog("params.c parse_param(): fail to parse conf file, "
	  //"param should in ONE line, i=%d,end=%d", i, end);
	  fprintf(stderr, "@parse_param(): fail to parse conf file, "\
		  "param should in single line\n");
	  return 1;

	case '\0':
	  //fsslog("params.c parse_param(): fail to parse conf file, "
	  // "EOF should not in parameter");
	  fprintf(stderr, "@parse_param(): fail to parse conf file, "\
		  "EOF should not in param\n");
	  return 1;

	default:
	  if (isspace(c))
	    c = skip_space(file);
	  else
	    {
	      param[i++] = c;
	      end = i;
	      c = getc(file);
	    }
	}
    }

   
  if (0 != strncmp(k, param, strlen(k))){
    c = skip_comment(file);
    return 2;
  }

  printf("KEY:%s found\n", k);

  i = 0;
  end = 0;

  c = skip_space(file);

  while ( (EOF!=c) && (c>0) ) {
    if (c == '\n') {
      value[end] = 0;
      break;
	  
    } else if (isspace(c)) {
      value[end] = 0;
      break;
	
    } else {
      value[i++] = c;
      end = i;
      c = getc(file);
    }
  }


  if (0 == strlen(value)) {
    v = NULL;
    printf("KEY:%s -> VALUE:NULL\n", k);
  }  else {
    if (NULL == strncpy(v, value, strlen(value))) {
      //fsslog("params.c parse_param(): fail to strncpy() to value, %s",
      //strerror(errno));
      perror("@parse_param(): fail to strncpy to *v");
      return 1;
    }
    v[strlen(value)] = 0;
    printf("KEY:%s -> VALUE:%s\n", k, v);
  }
  return 0;
  
}
	

  
