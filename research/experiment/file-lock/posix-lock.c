#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define FILENAME "posix-lock.c"

/**
 * F_SETLK   非阻塞版本
 * F_SETLKW  阻塞版本
 */
int main(int argc, char *argv[]){
    int fd, ret;
    char command[BUFSIZ];
    char *type = NULL, *prefix = NULL;
    struct flock lck;
    const char *file_name = NULL;
    int op = 0;

    if (argc >= 2)
        file_name = argv[1];
    else 
        file_name = FILENAME;

    if (argc >= 3)
        op = 1;

    /**
     * argv examine
     */
    if (access(file_name, R_OK & 0666) != 0){
        fprintf(stdout, "Usage: ./%s <file path> <bool>\n"
                "  file path:\n"
                "    posix-lock.c (default)\n"
                "  bool:\n"
                "    0: POSIX lock (default)\n"
                "    1: OFD lock(not works now)\n", argv[0]);
        return 0;
    }

    fd = open(file_name, O_RDWR & 0666);
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
                lck.l_type = F_UNLCK;

                ret = fcntl(fd, F_SETLK, &lck);
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
