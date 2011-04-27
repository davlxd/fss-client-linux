
#include "wrap-sha1.h"


int main()
{
  char *target = "/home/i/Desktop/new file";
  char *target_p = "/home/i/Desktop";
  char digest[41];
  char hash[42];
 

  sha1_str("new file", digest);
  printf("digest of new fil eis:--%s--\n", digest);

  sha1_digest_via_fname(target,  digest);
  printf("\n\n\ndigest_via_fname is:--%s--\n", digest);

  sha1_digest_via_fname_fss(target, target_p, digest);
  printf("\n\n\ndigest of old fss is--%s--\n", digest);

  compute_hash(target, target_p, digest, hash);
  printf("\n\n\nsha1 of fss new is--%s--\n", digest);

  printf("\n\n\nhash of fss new  is--%s--\n", hash);

  return 0;
}
