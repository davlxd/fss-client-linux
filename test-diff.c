#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HASH_LEN 40

int main(int argc, char **argv)
{
  diff("remote.hash.fss", "hash.fss", "diff.remote.index",
       "diff.local.index", NULL);


  return 0;

}
  
