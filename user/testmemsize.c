#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user/user.h"

int
main()
{
  uint64 before = getmemsize();
  char *p = sbrk(4096);
  uint64 after = getmemsize();

  if (!p) {
    printf("sbrk failed\n");
    exit(1);
  }
  printf("before : %lu\nafter allocating 4096 bytes to the heap : %lu\n ",
         before, after);
  return 0;
}
