#include "fss.h"

int main()
{
  char test[MAX_PATH_LEN];
  printf("MAX is %d\n", MAX_PATH_LEN);
  update_rootpath("/home/");
  rela2full("ada/tt/", test);
  printf("test is --%s--\n", test);

  return 0;


}
