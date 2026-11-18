#include "kernel/types.h"
#include "kernel/riscv.h"
#include "user/user.h"

#ifndef SUPERPGSIZE
#define SUPERPGSIZE (2 * 1024 * 1024)
#endif
#ifndef SUPERPGROUNDUP
#define SUPERPGROUNDUP(a) (((a) + SUPERPGSIZE - 1) & ~(SUPERPGSIZE - 1))
#endif

void
err(char *why)
{
  printf("superpgtest: %s failed\n", why);
  exit(1);
}

// grow the heap so that the returned address is 2MB-aligned and
// a full 2MB region starts there (this triggers a superpage).
char *
alloc_super(void)
{
  uint64 cur = (uint64)sbrk(0);
  uint64 s = SUPERPGROUNDUP(cur);
  if (s != cur && sbrk(s - cur) == (char *)-1)
    err("sbrk pad");
  if (sbrk(SUPERPGSIZE) == (char *)-1)
    err("sbrk super");
  return (char *)s;
}

void
fill(char *s)
{
  for (int i = 0; i < SUPERPGSIZE; i += PGSIZE)
    *(int *)(s + i) = i ^ 0x5a5a;
}

void
verify(char *s, int len, char *what)
{
  for (int i = 0; i < len; i += PGSIZE)
    if (*(int *)(s + i) != (i ^ 0x5a5a))
      err(what);
}

// 1. fresh superpage must be all zeros, then hold data on every 4KB page
void
test_basic(void)
{
  printf("test_basic\n");
  char *s = alloc_super();
  for (int i = 0; i < SUPERPGSIZE; i += 4)
    if (*(int *)(s + i) != 0)
      err("not zeroed");
  fill(s);
  verify(s, SUPERPGSIZE, "wrong value");
  sbrk(-SUPERPGSIZE);
}

// 2. copyin/copyout through a pipe, buffer in the middle of the superpage,
//    crossing a 4KB boundary
void
test_syscall(void)
{
  printf("test_syscall\n");
  char *s = alloc_super();
  char *src = s + 5 * PGSIZE + 4000; // crosses into the next page
  char *dst = s + 300 * PGSIZE + 100;
  int fd[2];

  for (int i = 0; i < 300; i++)
    src[i] = i;
  if (pipe(fd) < 0)
    err("pipe");
  if (write(fd[1], src, 300) != 300)
    err("write");
  if (read(fd[0], dst, 300) != 300)
    err("read");
  for (int i = 0; i < 300; i++)
    if (dst[i] != (char)i)
      err("syscall data mismatch");
  close(fd[0]);
  close(fd[1]);
  sbrk(-SUPERPGSIZE);
}

// 3. fork copies the superpage; child writes must not affect the parent
void
test_fork(void)
{
  printf("test_fork\n");
  char *s = alloc_super();
  fill(s);

  int pid = fork();
  if (pid < 0)
    err("fork");
  if (pid == 0) {
    verify(s, SUPERPGSIZE, "child sees wrong data");
    for (int i = 0; i < SUPERPGSIZE; i += PGSIZE)
      *(int *)(s + i) = 0;
    exit(0);
  }
  int st;
  wait(&st);
  if (st != 0)
    err("child failed");
  verify(s, SUPERPGSIZE, "parent corrupted by child");
  sbrk(-SUPERPGSIZE);
}

// 4. shrinking by one 4KB page splits the superpage; the rest must survive
void
test_partial(void)
{
  printf("test_partial\n");
  char *s = alloc_super();
  fill(s);

  sbrk(-PGSIZE); // forces demote
  verify(s, SUPERPGSIZE - PGSIZE, "data lost after partial free");

  if (sbrk(PGSIZE) == (char *)-1) // grow again: fresh page must be zero
    err("regrow");
  if (*(int *)(s + SUPERPGSIZE - PGSIZE) != 0)
    err("regrown page not zero");
  sbrk(-SUPERPGSIZE);
}

// 5. superpages must go back to the free list (no leak)
void
test_leak(void)
{
  printf("test_leak\n");
  for (int i = 0; i < 40; i++) { // 40 > 16 superpages available
    char *s = alloc_super();
    s[0] = 1;
    s[SUPERPGSIZE - 1] = 1;
    sbrk(-SUPERPGSIZE);
  }
}

int
main(void)
{
  test_basic();
  test_syscall();
  test_fork();
  test_partial();
  test_leak();
  printf("superpgtest: all tests passed\n");
  exit(0);
}
