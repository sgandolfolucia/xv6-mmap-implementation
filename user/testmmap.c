#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"
#include "kernel/syscall.h"
#include "kernel/memlayout.h"
#include "kernel/riscv.h"

int main() {
    int fd = open("hi.txt", O_CREATE|O_RDWR);
    write(fd, "hi", 2);

    void *addr = mmap(0,2, PROT_READ|PROT_WRITE,0,fd,0);
    int munny = munmap(0,0);

    printf("PRINTING %c %d\n", *((char *)addr), munny);

    // int *adr = (int *)10000000;
    // *adr = 5;
    close(fd);
    return 0;
}