#include <stdint.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <libdlm.h>
#include <pthread.h>
#include <linux/dlmconstants.h>

#define _REENTRANT

#define LOCKSPACE_NAME "TEST"
#define LOCKSPACE_MODE 0600
#define NAME "wow"

struct lock_wait {
    pthread_cond_t  cond;
    pthread_mutex_t mutex;
    struct dlm_lksb lksb;
};

static void sync_ast_routine(void *arg)
{
    struct lock_wait *lwait = arg;

    pthread_mutex_lock(&lwait->mutex);
    pthread_cond_signal(&lwait->cond);
    pthread_mutex_unlock(&lwait->mutex);
}


int lock_ls_resource(dlm_lshandle_t ls, const char *resource, int mode, int flags, int *lockid)
{
    int status;
    struct lock_wait lwait;

    if (ls == NULL){
        return -1;
    }

    /* Conversions need the lockid in the LKSB */
    if (lockid && (flags & LKF_CONVERT)){
        lwait.lksb.sb_lkid = *lockid;
    }
    else if(!lockid && (flags & LKF_CONVERT)){
        errno = EINVAL;
        return -1;
    }

    pthread_cond_init(&lwait.cond, NULL);
    pthread_mutex_init(&lwait.mutex, NULL);
    pthread_mutex_lock(&lwait.mutex);

    status = dlm_ls_lock(ls, mode,
              &lwait.lksb,
              flags,
              resource,
              strlen(resource),
              0,
              sync_ast_routine,
              &lwait,
              NULL,
              NULL);
    if (status)
        return status;

    /* Wait for it to complete */
    pthread_cond_wait(&lwait.cond, &lwait.mutex);
    pthread_mutex_unlock(&lwait.mutex);

    *lockid = lwait.lksb.sb_lkid;

    errno = lwait.lksb.sb_status;
    if (lwait.lksb.sb_status)
        return -1;
    else
        return 0;
}

int unlock_ls_resource(dlm_lshandle_t ls, int lockid)
{
    int status;
    struct lock_wait lwait;

    if (ls == NULL){
        errno = -ENOTCONN;
        return -1;
    }

    pthread_cond_init(&lwait.cond, NULL);
    pthread_mutex_init(&lwait.mutex, NULL);
    pthread_mutex_lock(&lwait.mutex);

    status = dlm_ls_unlock(ls, lockid, 0, &lwait.lksb, &lwait);

    if (status)
        return status;

    /* Wait for it to complete */
    pthread_cond_wait(&lwait.cond, &lwait.mutex);
    pthread_mutex_unlock(&lwait.mutex);

    errno = lwait.lksb.sb_status;
    if (lwait.lksb.sb_status != DLM_EUNLOCK)
        return -1;
    else
        return 0;
}

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

    /* key */
    if(dlm_ls_pthread_init(ls)){
        fprintf(stderr, "dlm_ls_pthread_init --> %s", strerror(errno));
        return 0;
    }

    int ret;
    int op, opmode, lkid;
    uint32_t mode;
    struct dlm_lksb lksb;

    /*
    dlm_lock_wait(LKM_NLMODE, &lksb, LKF_EXPEDITE,
                  NAME, strlen(NAME),
                  0, NULL, NULL, NULL);
    */

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
              //  ret = lock_ls_resource(ls, NAME, LKM_NLMODE, LKF_EXPEDITE, &lkid);
              //  printf("lock_ls_resource sucess, mode:NLMODE, lkid: %d\n", lkid);
                ret = dlm_ls_lock_wait(ls, LKM_NLMODE, &lksb, LKF_EXPEDITE|LKF_NODLCKWT, NAME, strlen(NAME), 0, NULL, NULL, NULL);
                if(ret != 0){
                    fprintf(stderr, "line %d --> lock_ls_resource: %s\n", __LINE__, strerror(errno));
                    break;
                }
                printf("lock_ls_resource sucess, mode:NLMODE, lkid: %d\n", lksb.sb_lkid);
                printf("%s\n", "CONVERT");
                ret = dlm_ls_lock_wait(ls, mode, &lksb,  LKF_CONVERT|LKF_NOQUEUE, NAME, strlen(NAME), 0, NULL, NULL, NULL);
                if(ret != 0){
                    fprintf(stderr, "line %d --> dlm_ls_lock_wait: %s\n", __LINE__, strerror(errno));
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
