#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#define RESOURCE_NAME      "RESOURCE_NAME"
#define LOCK_ID            "LOCK_ID"
#define LOCK_MODE          "LOCK_MODE"
#define QEMU_PID           "QEMU_PID"

int main(int argc, char *argv[]){
    int fd = open("/tmp/test-write", O_RDWR | O_CREAT | O_TRUNC);
    if(fd < 0){
        perror("open");
        return 0;
    }
    
    char buf[BUFSIZ] = {0};

    snprintf(buf, BUFSIZ-1, "%10d\n%32s %10s %10s %10s\n", getpid(), RESOURCE_NAME, LOCK_ID, LOCK_MODE, QEMU_PID);
    write(fd, buf, strlen(buf));

    snprintf(buf, BUFSIZ-1, "%32s %10d %10s %10d\n", "1234567890098765432112", 123456, "123456", 123456);
    write(fd, buf, strlen(buf));
    write(fd, buf, strlen(buf));
    write(fd, buf, strlen(buf));
    snprintf(buf, BUFSIZ-1, "%32s %10d %10s %10d\n", "123", 1, "1", 1);

    FILE *fp = fdopen(fd, "r+");
    if(fp == NULL){
        perror("fdopen");
        return 0;
    }

    fseek(fp, 40+32+3+2, SEEK_SET);
    write(fd, buf, strlen(buf));
    
    long offset;
    offset = 40+32+3+2+(30+32+3+1)*2;
    fsetpos(fp, &offset);
    write(fd, buf, strlen(buf));

    offset = 40+32+3+2+(30+32+3+1)*7;
    fsetpos(fp, &offset);
    snprintf(buf, BUFSIZ-1, "%32s %10d %10s %10d\n", "123456789", 123456, "123456", 123456);
    write(fd, buf, strlen(buf));
    return 0;
}
