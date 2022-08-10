#include "kernel/types.h"
#include "kernel/memlayout.h"
#include "user/user.h"

#define PID_PRINTF(fmt, ...) printf("[%d]" fmt, getpid(), ## __VA_ARGS__)

void copytest() {
    uint64 phys_size = PHYSTOP - KERNBASE;
    int sz = (phys_size / 3) * 2;

    PID_PRINTF("copy test\n");

    char *p = sbrk(sz);
    if(p == (char *)0xffffffffffffffffL) {
        PID_PRINTF("sbrk(%d) failed\n", sz);
    }

    int pid = fork();

    if(pid < 0) {
        PID_PRINTF("fork failed\n");
        exit(-1);
    }

    if(pid == 0) {
        exit(0);
    }

    wait(0);

    if(sbrk(-sz) == (char *)0xffffffffffffffffL) {
        PID_PRINTF("sbrk(-%d) failed\n", sz);
        exit(-1);
    }

    PID_PRINTF("copy test finished\n");
    return;
}

// three processes all write COW memory.
// this causes more than half of physical memory
// to be allocated, so it also checks whether
// copied pages are freed.
void
threetest()
{
  uint64 phys_size = PHYSTOP - KERNBASE;
  int sz = phys_size / 4;
  int pid1, pid2;

  PID_PRINTF("three: ");
  
  char *p = sbrk(sz);
  if(p == (char*)0xffffffffffffffffL){
    PID_PRINTF("sbrk(%d) failed\n", sz);
    exit(-1);
  }

  pid1 = fork();
  if(pid1 < 0){
    PID_PRINTF("fork failed\n");
    exit(-1);
  }
  if(pid1 == 0){
    pid2 = fork();
    if(pid2 < 0){
      PID_PRINTF("fork failed");
      exit(-1);
    }
    if(pid2 == 0){
      for(char *q = p; q < p + (sz/5)*4; q += 4096){
        *(int*)q = getpid();
      }
      for(char *q = p; q < p + (sz/5)*4; q += 4096){
        if(*(int*)q != getpid()){
          PID_PRINTF("wrong content\n");
          exit(-1);
        }
      }
      exit(-1);
    }
    for(char *q = p; q < p + (sz/2); q += 4096){
      *(int*)q = 9999;
    }
    exit(0);
  }

  for(char *q = p; q < p + sz; q += 4096){
    *(int*)q = getpid();
  }

  wait(0);

  sleep(1);

  for(char *q = p; q < p + sz; q += 4096){
    if(*(int*)q != getpid()){
      PID_PRINTF("wrong content\n");
      exit(-1);
    }
  }

  if(sbrk(-sz) == (char*)0xffffffffffffffffL){
    PID_PRINTF("sbrk(-%d) failed\n", sz);
    exit(-1);
  }

  PID_PRINTF("ok\n");
}

char junk1[4096];
int fds[2];
char junk2[4096];
char buf[4096];
char junk3[4096];

// test whether copyout() simulates COW faults.
void
filetest()
{
  PID_PRINTF("file: ");
  
  buf[0] = 99;

  for(int i = 0; i < 2; i++){
    if(pipe(fds) != 0){
      PID_PRINTF("pipe() failed\n");
      exit(-1);
    }
    int pid = fork();
    if(pid < 0){
      PID_PRINTF("fork failed\n");
      exit(-1);
    }
    if(pid == 0){
      sleep(1);
      int nbytes = read(fds[0], buf, sizeof(i));
      if(nbytes != sizeof(i)){
        PID_PRINTF("read: %d\n", nbytes);
        PID_PRINTF("error[%d]: read failed\n", getpid());
        exit(1);
      }
      sleep(1);
      int j = *(int*)buf;
      if(j != i){
        PID_PRINTF("error: read the wrong value\n");
        exit(1);
      }
      exit(0);
    }
    PID_PRINTF("start write\n");
    if(write(fds[1], &i, sizeof(i)) != sizeof(i)){
      PID_PRINTF("error: write failed\n");
      exit(-1);
    }
    else {
      PID_PRINTF("write ok: %d\n", i);
    }
  }

  int xstatus = 0;
  for(int i = 0; i < 2; i++) {
    wait(&xstatus);
    if(xstatus != 0) {
      exit(1);
    }
  }

  if(buf[0] != 99){
    PID_PRINTF("error: child overwrote parent\n");
    exit(1);
  }

  PID_PRINTF("ok\n");
}

int main() {
    /*
    copytest();
    copytest();
    copytest();

    threetest();
    threetest();
    threetest();
    */

    filetest();
    PID_PRINTF("all test successful.\n");
    exit(0);
}