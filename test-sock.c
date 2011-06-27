#include <fcntl.h>
#include <stdio.h>
#include "sock.h"


int main()
{
  int fd;
  fd = open("../output", O_RDONLY);

  char buf[1024];
  read_msg_head(fd, buf, 1024);

  printf("-----%s-----\n", buf);



  close(fd);
  
  



  return 0;
}
