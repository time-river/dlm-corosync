#include <stdint.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <libdlm.h>

#define DEBUG
#define LOCKNAME "wow"
#define LOCKID   0

#ifdef DEBUG
#define DEBUG_LOG(log, ...) \
    fprintf(stdout, "%s %d %s: " #log "\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__)
#else
#define DEBUG_LOG(log, ...)
#endif

static void ret_log(int ret, char *name, uint32_t lockid){
    if (ret != 0) {
        DEBUG_LOG("%s : %s", name, strerror(errno));
    }
    else {
        DEBUG_LOG("success, lockid: %d", lockid);
    }

    return;
}

int main(void) {
    char resource[BUFSIZ];
    int op, opmode, opname, opid;
    int mode;
    int flags;
    uint32_t lockid;
    int ret;

    while(true){
        memset(resource, 0, sizeof(resource));
        

        scanf("%d %d %d %d", &op, &opname, &opmode, &opid);

        switch(opname){
            case 0:
                DEBUG_LOG("resource: %s", LOCKNAME);
                strcpy(resource, LOCKNAME);
                break;
            case 1:
                DEBUG_LOG("resource: %s", "TEST");
                break;
            case 2:
                DEBUG_LOG("resource: %s", "FUCK");
        }

        switch(opmode){
            case 0:
                DEBUG_LOG("mode: LKM_NLMODE");
                mode = LKM_NLMODE;
                break;
            case 1:
                DEBUG_LOG("mode: LKM_CRMODE");
                mode = LKM_CRMODE;
                break;
            case 2:
                DEBUG_LOG("mode: LKM_CWMODE");
                mode = LKM_CWMODE;
                break;
            case 3:
                DEBUG_LOG("mode: LKM_PRMODE");
                mode = LKM_PRMODE;
                break;
            case 4:
                DEBUG_LOG("mode: LKM_PWMODE");
                mode = LKM_PWMODE;
                break;
            case 5:
                DEBUG_LOG("mode: LKM_EXMODE");
                mode = LKM_EXMODE;
                break;
            default:
                DEBUG_LOG("default mode: LKM_NLMODE");
                mode = LKM_NLMODE;
                break;
        }

        DEBUG_LOG("%d", opid);
        lockid = opid;

        switch(op){
            case 1: // lock
                DEBUG_LOG("resource: %s, mode: %x, flags: 0, lockid: %x", resource, mode, lockid);
                ret = lock_resource(resource, mode, 0, &lockid);
                ret_log(ret, "lock_resource", lockid);
                break;
            case 2: // unlock
                DEBUG_LOG("resource: %s, mode: %x, flags: 0, lockid: %x", resource, mode, lockid);
                ret = unlock_resource(lockid);
                ret_log(ret, "unlock_resource", lockid);
                break;
            case 3:
                fprintf(stdout, "exit");
                return 0;
            default:
                fprintf(stderr, "input error\n");
                continue;
                break;
        }   
    }

    return 0;
}
