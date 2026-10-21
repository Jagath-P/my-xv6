#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{

  if (argc < 3) {
    printf("Format is \"tracer <mask> <cmd> \" \n");
    exit(1);
  }
  trace(atoi(argv[1]));

  exec(argv[2], argv + 2);
}
