#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"

int
main(int argc, char *argv[])
{

  int child = fork();
  if (child < 0) {
    printf("Fork failed\n");
    exit(1);
  }
  if (child == 0) {
    printf("Iam child! PID: %d\n", getpid());
    exit(0);
  } else {
    int status;
    int pid = wait(&status);
    int ut = uptime();
    printf("Parent process id: %d\nUptime : %d\n", getpid(), ut);

    printf("Child: %d\n", pid);
    printf("Exit status: %d\n", status);
    return 0;
  }
}
