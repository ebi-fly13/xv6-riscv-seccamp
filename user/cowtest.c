#include "kernel/types.h"
#include "kernel/memlayout.h"
#include "user/user.h"

void copytest() {
    uint64 phys_size = PHYSTOP - KERNBASE;
    int sz = (phys_size / 3) * 2;

    printf("copy test\n");

    char *p = sbrk(sz);
    if(p == (char *)0xffffffffffffffffL) {
        printf("sbrk(%d) failed\n", sz);
    }

    int pid = fork();

    if(pid < 0) {
        printf("fork failed\n");
        exit(-1);
    }

    if(pid == 0) {
        exit(0);
    }

    wait(0);

    if(sbrk(-sz) == (char *)0xffffffffffffffffL) {
        printf("sbrk(-%d) failed\n", sz);
        exit(-1);
    }

    printf("copy test finished\n");
    return;
}

int main() {
    copytest();
    printf("all test successful.\n");
    return 0;
}