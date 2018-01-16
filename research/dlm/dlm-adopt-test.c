/*
 * 1. is dlm running ?
 * 2. does lockspace exist?
 * 
 * lockspace not exist:
 *   3. create lockspace
 *   4. create 'dlmlock' file
 *   5. initialize 'dlmlock' file
 *   6. initialize lock_info list
 * lockspace exist:
 *   3. open lockspace
 *   4. does 'dlmlock' file exist?
 *  'dlmlock' file not exist:
 *    5. create 'dlmlock' file
 *    6. initialize 'dlmlock' file
 *    7. initialize 'lock_info' list
 *  'dlmlock' file exist:
 *    5. initialize 'lock_info' list
 *    6. read 'dlmlock' file
 *    7. adopt orphan lock, stored in 'lock_info' list
 *
 * complete
 */


#include <stdint.h>
#include <errno.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <libdlm.h>
#include <corosync/cpg.h>
#include "list.h"

/* This will be set after dlm_controld is started. */
#define DLM_CLUSTER_NAME_PATH "/sys/kernel/config/dlm/cluster/cluster_name"
#define DLM_FILE_PATH         "/tmp/dlmfile"
#define LOCKSPACE_NAME     "ADOPT_TEST"
#define LOCKSPACE_MODE     0600
#define DLM_FILE           "dlmlock"
#define RESOURCE_NAME_HASH "RESOURCE_NAME_HASH"
#define QEMU_PID           "QEMU_PID"
#define LOCK_ID            "LOCK_ID"
#define LOCK_MODE          "LOCK_MODE"

#define LEN 33
struct lock_info {
    struct list_head list;
    char   name[LEN];
    int    qpid;
    int    lkid;
    mode_t mode;
};

static dlm_lshandle_t lockspace = NULL;
static int dlmfile_fd;
static struct list_head lock_list;

int dlm_is_running(void);
int setup_lockspace(void);
int setup_dlmfile(bool new);
int lock_dump(int fd, ...);
int file_init(int fd);
ssize_t safewrite(int fd, const void *buf, size_t count);
unsigned int get_local_nodeid(void);

int main(void){
    INIT_LIST_HEAD(&lock_list);

    return 0;
}


int dlm_is_running(void){
    int ret;

    /* check dlm_controld */
    ret = access(DLM_CLUSTER_NAME_PATH, F_OK);
    if (ret < 0) {
        fprintf(stderr, "%s: %s\n", __func__, strerror(errno));
        ret = 0;
    } else {
        ret = 1;
    }

    return ret;
}

int setup_lockspace(void){
    lockspace = dlm_create_lockspace(LOCKSPACE_NAME, LOCKSPACE_MODE);
    if(!lockspace)
        return 1;
    else if(lockspace && errno == -EEXIST){
        lockspace = dlm_open_lockspace(LOCKSPACE_NAME);
        if(!lockspace){
            return 2;
        }
        else{
            fprintf(stderr, "%s: %s\n", __func__, strerror(errno));
            return -1;
        }
    }
    else{
        fprintf(stderr, "%s: %s\n", __func__, strerror(errno));
        return -1;
    }
}

int setup_dlmfile(bool new){
    FILE *fp;
    int flags = O_RDWR | O_SYNC;
    pid_t previous_pid;
    int count;
    struct lock_info raw_lock;
    struct lock_info *lock;

    if(new || access(DLM_FILE_PATH, F_OK) != 0){
        flags |= (O_CREAT | O_TRUNC);
        dlmfile_fd = open(DLM_FILE_PATH, flags);        
        if(dlmfile_fd < 0){
            fprintf(stderr, "%s: %s\n", __func__, strerror(errno));
            return -1;
        }
        else{
            file_init(dlmfile_fd);
            return 0;
        }
    }
    else if(access(DLM_FILE_PATH, F_OK) == 0){
        dlmfile_fd = open(DLM_FILE_PATH, flags);
        if (dlmfile_fd < 0){
            fprintf(stderr, "%s: %s\n", __func__, strerror(errno));
            return -1;
        }

        fp = fdopen(dlmfile_fd, "r+");
        if(fp == NULL){
            fprintf(stderr, "%s: %s\n", __func__, strerror(errno));
            return -1;
        }
        else{
            count = fscanf(fp, "%d\n", &previous_pid);
            while(count != 0){
                count = fscanf(fp, "%s %d %d %d\n", raw_lock.name, &raw_lock.qpid, &raw_lock.lkid, &raw_lock.mode);
                if(count == 0)
                    break;

                adopt_lock(raw_lock);
            }
        }
    }
}

int adopt_lock(struct lock_info raw_lock){
    struct lock_info *lock;
    uint32_t flags =  LKF_PERSISTENT |  LKF_ORPHAN;
    struct dlm_lksb lksb = {
        .sb_lkid = raw_lock.lkid,
        .sb_lvbptr = NULL,
    };
    int rv;


    /*
     * qemu process exist?
     * not exist:
     *   release lock
     * exist:
     */
    rv = dlm_ls_lockx(lockspace, raw_lock.mode, &lksb, flags,
              raw_lock.name, strlen(raw_lock.name), 0,
              (void *)1, (void *)1, (void *)1,
              NULL, NULL);

    if (rv == -1 && errno == -EAGAIN) {
        rv = -EUCLEAN;
        fprintf(stderr, "%s: %s, try again\n", __func__, strerror(errno));
        return -1;
    }
    else if (rv < 0) {
        fprintf(stderr, "%s: %s\n", __func__, strerror(errno));
        return -1;
    }


    lock = calloc(sizeof(struct lock_info), 1);
    if(lock == NULL){
        fprintf(stderr, "%s: %s\n", __func__, strerror(errno));
        return -1;
    }

    memmove(lock->name, raw_lock.name, LEN);
    lock->qpid = raw_lock.qpid;
    lock->lkid = lksb.sb_lkid;
    lock->mode = raw_lock.mode;
}

int file_init(int fd){
    dprintf(fd, "%d\n", getpid());
    safewrite(fd, getpid(), sizeof(pid_t));
    dprintf(fd, "%s %s %s %s\n", RESOURCE_NAME_HASH, QEMU_PID, LOCK_ID, LOCK_MODE);

    return 0;
}


unsigned int get_local_nodeid(void){
    cpg_handle_t handle;
    int result;
    unsigned int nodeid;

    result = cpg_model_initialize(&handle, CPG_MODEL_V1, NULL, NULL);
    cpg_local_get(handle, &nodeid);
    printf("nodeid: %u\n", nodeid);
    cpg_finalize(handle);

    return nodeid;
}
