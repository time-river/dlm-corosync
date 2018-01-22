#define _REENTRANT
#include <sys/types.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <libdlm.h>

#define LOCKSPACE_NAME "close-test"
#define LOCKSPACE_MODE 0600
#define LOCK_NAME   "lock"

static dlm_lshandle_t ls;

int main(int argc, char *argv[]){
    int ret;

    ls = dlm_open_lockspace(LOCKSPACE_NAME);
    if (!ls)
        ls = dlm_create_lockspace(LOCKSPACE_NAME, LOCKSPACE_MODE);
    if (!ls)
        return 0;
    dlm_ls_pthread_init(ls);

    struct dlm_lksb lksb;
    
    ret = dlm_ls_lockx(ls, LKM_EXMODE, &lksb, LKF_PERSISTENT|LKF_ORPHAN,
            LOCK_NAME, strlen(LOCK_NAME), 0,
            (void *)1, (void *)1, (void *)1,
            NULL, NULL);
    printf("lk_status --> %d\n", lksb.sb_status);
    if(!ret)
        printf("adopt success. lkid --> %d\n", lksb.sb_lkid);

    ret = dlm_ls_lock_wait(ls, LKM_EXMODE, &lksb, LKF_PERSISTENT|LKF_NOQUEUE, LOCK_NAME, strlen(LOCK_NAME), 0, NULL, NULL, NULL);
    printf("lk_status --> %d\n", lksb.sb_status);
    if(!ret && !lksb.sb_status)
        printf("lock success. lkid --> %d\n", lksb.sb_lkid);
    else{
        perror("dlm_ls_lock_wait failed");
    }

    dlm_close_lockspace(ls);

    return 0;
}
