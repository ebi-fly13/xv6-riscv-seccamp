#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
    int size = freemem();
    if (size < 0) {
        exit(1);
    }
    printf("%d\n", size);
    exit(0);
}