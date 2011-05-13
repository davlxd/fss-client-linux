#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HASH_LEN 40

int main(int argc, char **argv)
{
  int linenum = 0;
  
  if (search_line("hash.fss", "91208DE65657A7F7135FB5C7DD05E600DF180634", 41, &linenum)) {
    fprintf(stderr, "@main(): search_line() failed\n");
    return 1;
  }



  printf("--%d--\n", linenum);
  
    

 


  return 0;

}
  
