#include <sys/stat.h>
#include <errno.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <libdlm.h>

#define DEBUG
#define LOCKSPACENAME "lock"
#define LOCKSAPCEMODE 0600
#define LOCKNAME "wow"

#ifdef DEBUG
#define DEBUG_LOG(log, ...) \
    sprintf(stdout, "%s %s %s: " #log "\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__)
#else
#define DEBUG_LOG(log, ...)
#endif

#define DLM_FUNC(func, ...) func(__VA_ARGS__)

static void ret_log(int ret, char *name, struct dlm_lksb *lksb){
    if (ret != 0) {
        DEBUG_LOG("%s : %s", name, strerror(errno));
    }
    else {
        sprintf(stdout, "%s sucess.\n", name);
        if (lksb)
            sprintf("dlm_lksb information:\n"
                "   sb_status: %x\n"
                "   sb_lkid: %x\n"
                "   sb_flags: %x\n"
                "   sblvbptr: %s\n",
                lksb->sb_status, lksb->sb_lkid, lksb->sb_flags, lksb->sb_lvbptr);
    }

    return;
}

int main(int argc, char *argv[]){
    dlm_lshandle_t ls;
    struct dlm_lksb lksb;
    char *name = LOCKNAME;
    mode_t mode;
    char input[BUFSIZ];
    int ret;
    int (*callback)(dlm_lshandle_t, ...);

    DEBUG_LOG("dlm_create_lockspace"); 
    ls = dlm_create_lockspace(LOCKSPACENAME, LOCKSAPCEMODE);
    if (ls != 0 && errno == -EEXIST) {
        DEBUG_LOG("dlm_open_lockspace");
        ls = dlm_open_lockspace(LOCKSPACENAME);
        if (ls != 0) {
            DEBUG_LOG("dlm_open_lockspace: %s", strerror(errno));
            return -1;
        }
    }
    else if (ls != 0){
        DEBUG_LOG("dlm_create_lockspace: %s", strerror(errno));
        return -1;
    }


    while(true) {
        memset(input, 0, sizeof(input));
        memset(&lksb, 0, sizeof(lksb));

        scanf("%s", input);

        name = strtok(input, ' ');
        if(!name)
            name = LOCKNAME;

        /* for lock & unlock */
        switch(input[1]) {
            case '0':
                mode = LKM_NLMODE;
                break;
            case '1':
                mode = LKM_CRMODE;
                break;
            case '2':
                mode = LKM_CWMODE;
                break;
            case '3':
                mode = LKM_PRMODE;
                break;
            case '4':
                mode = LKM_PWMODE;
                break;
            case '5':
                mode = LKM_EXMODE;
                break;
        }

        switch(input[0]){
            case 'a': // acqiure lock
                DEBIG_LOG("dlm_ls_lock_wait, name: %s", name);
                ret = dlm_ls_lock_wait(ls, mode, &lksb, 0,
                        name, strlen(name), 
                        0, NULL, NULL, NULL);
                ret_log(ret, "dlm_ls_lock_wait", &lksb);
                break;
            case 'r': // release lock
                DEBIG_LOG("dlm_ls_unlock_wait");
                ret = dlm_ls_unlock_wait(ls, mode, &lksb, );
                ret_log(ret, "dlm_ls_unlock_wait");
                break;
            case 'c': // close lockspace
                DEBUG_LOG("dlm_close_lockspace");
                ret = dlm_close_lockspace(ls);
                ret_log(ret, "dlm_close_lockspace");
                break;
            case 'd':
                DEBUG_LOG("dlm_release_lockspace, force");
                ret = dlm_release_lockspace(name);
                ret_log(ret, "dlm_release_lockspace");
            case 'f':
                DEBUG_LOG("fork");
                pid_t pid = fork();
                if(fork == 0) {
                
                }
                else {
                    sprintf(stdout, "parent: parent sleep 10s, then close lockspace, after 10s, then release lockspace, after 10s, then exit\n");
                    sleep(30);
                    eixt(0);
                }
            case 'q':
                sprintf(stdout, "exit\n");
                return 0;
            default:
                sprintf(stderr, "not correctly selection.");
                continue;
        }

        

            return -1;
        }

    }

    return 0;
}
