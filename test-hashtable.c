#include "hashtable.h"
#include "sock.h"
#include "stdio.h"
#include <fcntl.h>
#include <inttypes.h>
#include <sys/stat.h>


int main(int argc, char **argv)
{
  int fd;
  char buf[1024];

  file_struct *ffss;

  fd = open("../output", O_RDONLY);

  fss_readline(fd, buf, 1024);

  if (strncmp(buf, "M", 1) == 0) {
    receive_blk_checksums(fd, &ffss);

    printf("----%s-----\n", ffss->relaname);
    check_blk_structs();
    free_hashtable();
  }

  close(fd);

  return 0;
}
