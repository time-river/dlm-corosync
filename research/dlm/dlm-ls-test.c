#include <stdint.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <libdlm.h>


#define LOCKSPACE_NAME "TEST"
#define LOCKSPACE_MODE 0600
#define NAME "wow"

int main(void){
    dlm_lshandle_t ls;

    ls = dlm_open_lockspace(LOCKSPACE_NAME);
    if (!ls){
        ls = dlm_create_lockspace(LOCKSPACE_NAME, LOCKSPACE_MODE);
        if (!ls){
            fprintf(stderr, "%d --> dlm_create_lockspace: %s\n", __LINE__, strerror(errno));
            return 0;
        }
    }

    int ret;
    int op, opmode, lkid;
    uint32_t mode;
    struct dlm_lksb lksb;

    while(true){
        printf("opmode(0-lock, 1-unlock) opmode(0-PRMODE, 1-EMODE) lkid\n");
        scanf("%d %d %d", &op, &opmode, &lkid);
        switch(opmode){
            case 0:
                mode = LKM_PRMODE;
                puts("LKM_PRMODE");
                break;
            default:
                mode = LKM_EXMODE;
                puts("LKM_EXMODE");
                break;
        }

        switch(op){
            case 0:
                puts("lock");
                ret = dlm_ls_lock_wait(ls, LKM_NLMODE, &lksb, LKF_EXPEDITE|LKF_NODLCKWT, NAME, strlen(NAME), 0, NULL, NULL, NULL);
                printf("NLMODE, lkid: %d\n", lksb.sb_lkid);
                if(ret != 0){
                    fprintf(stderr, "%d --> dlm_ls_lock_wait: %s\n", __LINE__, strerror(errno));
                    break;
                }
                printf("%s\n", "CONVERT");
                ret = dlm_ls_lock_wait(ls, mode, &lksb,  LKF_CONVERT|LKF_NOQUEUE, NAME, strlen(NAME), 0, NULL, NULL, NULL);
                if(ret != 0){
                    fprintf(stderr, "%d --> dlm_ls_lock_wait: %s\n", __LINE__, strerror(errno));
                    break;
                }
                printf("lock success, lkid: %d\n", lksb.sb_lkid);
                break;
            default:
                puts("unlock");
                ret = dlm_ls_unlock_wait(ls, lkid, 0, &lksb);
                if(ret != 0){
                    fprintf(stderr, "%d --> dlm_ls_lock_wait: %s\n", __LINE__, strerror(errno));
                    break;
                }
                break;
        }
    }


}
