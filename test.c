#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <stdarg.h>
#include "sock.h"


static int va_test(int size, const char *fmt, ...)
{
  char buf[size];
  va_list ap;
  va_start(ap, fmt);

  vsnprintf(buf, size, fmt, ap);


  printf("in va_test(), --%s--\n", buf);

  



  va_end(ap);




}

int main()
{
  int fd;
  char buf[20];
  strncpy(buf, "0123456789", 20);

  va_test(strlen(buf)+5, ">%s<", buf);
  
  return 0;
}
  
