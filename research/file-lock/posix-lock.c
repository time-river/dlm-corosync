#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <fcntl.h>

#define FILENAME "posix-lock.c"

/**
 * F_SETLK   非阻塞版本
 * F_SETLKW  阻塞版本
 */
int main(void){
    int fd, ret;
    char command[BUFSIZ];
    char *type = NULL, *prefix = NULL;
    struct flock lck;

    fd = open("posix-lock.c", O_RDWR & 0666);
    while(true){
        lck.l_whence = SEEK_SET;
        lck.l_start = 0;
        lck.l_len = 0;

        scanf("%s", command);
        switch (command[0]){
            case 'l':
                switch(command[1]){
                    case 'w':
                        lck.l_type = F_WRLCK;
                        break;
                    case 'r':
                        lck.l_type = F_RDLCK;
                        break;
                }

                ret = fcntl(fd, F_SETLK, &lck);
                break;
            case 'r':
                switch(command[1]){
                    case 'w':
                        lck.l_type = F_WRLCK;
                        break;
                    case 'r':
                        lck.l_type = F_RDLCK;
                        break;
                }

                ret = fcntl(fd, F_SETLKW, &lck);
                break;
            case 's':
                switch(command[1]){
                    case 'w':
                        lck.l_type = F_WRLCK;
                        break;
                    case 'r':
                        lck.l_type = F_RDLCK;
                        break;
                }

                ret = fcntl(fd, F_GETLK, &lck);
                break;
            default:
                close(fd);
                exit(0);
                break;
        }
        if(ret < 0)
            perror("fcntl");
        else{
            switch(lck.l_type){
                case F_RDLCK:
                    type = "F_RDLCK";
                    break;
                case F_WRLCK:
                    type = "F_WRLCK";
                    break;
                case F_UNLCK:
                    type = "F_UNLCK";
                    break;
                default:
                    type = "error";
                    break;
            }

            switch(command[0]){
                case 's':
                    prefix = "test";
                    break;
                case 'l':
                    prefix = "acquire";
                    break;
                case 'r':
                    prefix = "release";
                    break;
            }

            printf("%s result: %s\n", prefix, type);
        }
        memset(command, 0, sizeof(command));
        memset(&lck, 0, sizeof(lck));
    }

    return 0;
}
