/*
 * setup:
 *
 *  1. initialize lock_info lk_list
 *  2. is dlm running ?
 *  3. setup lockspace
 * 
 * lockspace not exist:
 *   . create lockspace
 * lockspace exist:
 *   . open lockspace
 *
 *  4. prepare to setup 'dlmlock' file
 *
 * 'dlmlock' file not exist
 *   . create 'dlmlock' file
 * 'dlmlock' file exist
 *   . open file
 *   . setup adopting lock
 *
 *  adopt lock:
 *    . parse every line of file
 *    . adopt lock
 *    . store orphan lock in lk_list, abandon lock not related qemu
 *
 *  5. initialize 'dlmlock' file
 *    . firstline: current process pid
 *    . secondline: <title>
 *    . after: lock information per line
 *
 * complete
 */

#define _REENTRANT // for `dlm_ls_pthread_init`

#include <stdint.h>
#include <errno.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <libdlm.h>
#include <corosync/cpg.h>
#include "list.h"

#define SHA256LEN 33

#define DLM_CLUSTER_NAME_PATH "/sys/kernel/config/dlm/cluster/cluster_name"
#define DLM_FILE_PATH         "/tmp/dlmfile"
#define LOCKSPACE_NAME     "ADOPT_TEST"
#define LOCKSPACE_MODE     0600
#define DLM_FILE           "dlmlock"
#define STATUS             "STATUS"
#define RESOURCE_NAME      "RESOURCE_NAME"
#define LOCK_ID            "LOCK_ID"
#define LOCK_MODE          "LOCK_MODE"
#define QEMU_PID           "QEMU_PID"

struct lock {
    struct list_head entry;
    char name[SHA256LEN];
    uint32_t mode;
    uint32_t lkid;
    pid_t  vm_pid;
};

static struct list_head lk_list;
static dlm_lshandle_t lockspace;

bool dlm_running(void);
bool setup_lockspace(void);
bool setup_dlm_lockfile(void);
bool write_lock(struct lock *lk);
bool write_unlock(uint32_t lkid);

char *to_mode_text(uint32_t mode);
struct lock *create_lock(struct lock *lk);
void free_lock(struct lock *lk);

int main(int argc, char *argv[]){
    list_head_init(&lk_list);

    if(!dlm_running()){
        fprintf(stdout, "dlm is not running.\n");
        return 0;
    }

    setup_lockspace();       
    setup_dlm_lockfile();

    int raw_name, raw_mode, op;
    pid_t vm_pid = 121;
    char *name;
    uint32_t mode, flags, lkid;
    struct dlm_lksb lksb;
    int ret, line;
    struct lock *lk;

    while(true){
        fprintf(stdout, "\ninput:\n name --> 0, 1\n mode --> 0(PR) 1(EX)\n op --> 0(lock) 1(unlock)\n lkid\n");
        scanf("%d %d %d %d", &raw_name, &raw_mode, &op, &lkid);
        switch(raw_name){
            case 0:
                name = "1234567890098765432112";
                break;
            case 1:
                name = "test";
                break;
            case 2:
                name = "wow";
                break;
            default:
                name = "default";
                break;
        }
        printf(" name --> %s\n", name);
        switch(raw_mode){
            case 0:
                mode = LKM_PRMODE;
                fprintf(stdout, " mode --> LKM_PRMODE\n");
                break;
            case 1:
            default:
                fprintf(stdout, " mode --> LKM_EXMODE\n");
                mode = LKM_EXMODE;
                break;
        }

        switch(op){
            case 0:
                fprintf(stdout, "action --> lock\n");
                /*
                ret = dlm_ls_lock_wait(lockspace, LKM_NLMODE, &lksb, LKF_EXPEDITE, name, strlen(name), 0, NULL, NULL, NULL);
                if(ret != 0){
                    fprintf(stderr, "%s: %s\n", __func__, strerror(errno));
                    break;
                }
                fprintf(stdout, "lock NL MODE\n");*/
                flags = LKF_PERSISTENT | LKF_NOQUEUE;
                memset(&lksb, 0, sizeof(lksb));
                //flags = LKF_CONVERT | LKF_NOQUEUE;
                ret = dlm_ls_lock_wait(lockspace, mode, &lksb, flags, name, strlen(name), 0, NULL, NULL, NULL); 
                if(ret != 0 || lksb.sb_status != 0){
                    fprintf(stderr, "%s: %s\n", __func__, strerror(errno));
                    //dlm_ls_unlock_wait(lockspace, lksb.sb_lkid, flags, &lksb);
                }
                else{
                    fprintf(stdout, "result --> lock success\n lkid --> %d, status --> %d\n", lksb.sb_lkid, lksb.sb_status);
//                    lk = create_lock(name, mode, lkid, vm_pid);
                    if(lk != NULL){
                        lk->mode = to_mode_text(mode);
                        lk->lkid = lksb.sb_lkid;
                        lk->vm_pid = vm_pid;
                        strncpy(lk->name, name, SHA256LEN);
                        write_lock(lk);
                    }
                    else{
                        fprintf(stderr, "%s: %s\n", __func__, strerror(errno));
                    }
                }
                break;
            default:
                fprintf(stdout, "action --> unlock\n");
                list_for_each_entry(lk, &lk_list, entry){
                    if((!strcmp(lk->name, name)) && (lk->lkid == lkid)){
                        flags = 0;
                        ret = dlm_ls_unlock_wait(lockspace, lkid, flags, &lksb);
                        if(ret != 0){
                            fprintf(stderr, "dlm_ls_unlock_wait: %s, name: %s lkid: %d, status: %d\n", strerror(errno), name, lkid, lksb.sb_status);
                        }
                        else{
                            fprintf(stdout, "unlock success, name --> %s, lkid --> %d, status: %d\n", name, lkid, lksb.sb_status);
                            write_unlock(lkid);
                        }
                        free_lock(lk);
                        break;
                    }
                }
                break;
        }
    }
    return 0;
}

bool dlm_running(void){
    int ret;

    /* check dlm_controld */
    ret = access(DLM_CLUSTER_NAME_PATH, F_OK);
    if (ret < 0){
        fprintf(stderr, "%s: %s\n", __func__, strerror(errno));
    }

    return ret==0 ? true : false;
}

bool setup_lockspace(void){
    lockspace = dlm_open_lockspace(LOCKSPACE_NAME);
    if (!lockspace)
        lockspace = dlm_create_lockspace(LOCKSPACE_NAME, LOCKSPACE_MODE);
    if (!lockspace)
        return false;
    
    if(dlm_ls_pthread_init(lockspace)){
        fprintf(stderr, "%s: %s\n", __func__, strerror(errno));
        if(errno != -EEXIST)
            return false;
    }
    return true;
}

char *to_mode_text(uint32_t mode){
    switch(mode){
        case LKM_PRMODE:
            return "LKM_PRMODE";
        case LKM_EXMODE:
            return "LKM_EXMODE";
    }
}

uint32_t to_mode_uint(const char * mode){
    if(strcmp(mode, "LKM_PRMODE") == 0)
        return LKM_PRMODE;
    else if(strcmp(mode, "LKM_EXMODE") == 0)
        return LKM_EXMODE;
    else
        return 0; // not use 0, stand for error
}

bool get_local_nodeid(unsigned int *nodeid){
    cpg_handle_t handle;
    cs_error_t result;

    if(geteuid() != 0){
        fprintf(stderr, "need superuser privileges\n");
        return false;
    }

    result = cpg_model_initialize(&handle, CPG_MODEL_V1, NULL, NULL);
    if(result != CS_OK){
        fprintf(stderr, "%s: %s\n", __func__, strerror(errno));
        return false;
    }
    result = cpg_local_get(handle, nodeid);
    if(result != CS_OK){
        fprintf(stderr, "%s: %s\n", __func__, strerror(errno));
        return false;
    }
    fprintf(stdout, "nodeid: %u\n", *nodeid);

    result = cpg_finalize(handle);
    if(result != CS_OK){
        fprintf(stderr, "%s: %s\n", __func__, strerror(errno));
    }

    return nodeid;
}

bool adopt_lock(char *raw){
    char *str, *subtoken, *saveptr;
    int i, ret;
    int status;
    char *name;
    pid_t vm_pid;
    uint32_t mode;
    uint32_t flags;
    struct dlm_lksb lksb = {
        .sb_lvbptr = NULL,
    };
    struct lock *lk;

    for(i=0, status=0, str=raw; ; str=NULL, i++){
        subtoken = strtok_r(str, " \n", &saveptr);
        if(subtoken == NULL)
            break;

        switch(i){
            case 0:
                status = atoi(subtoken);
                break;
            case 1:
                name = strdup(subtoken);
                break;
            case 2:
                lksb.sb_lkid = atoi(subtoken);
                if(lksb.sb_lkid == 0) // if non-digit, return 0. For example: '123 123' --> 123, 'abcd' --> 0
                    return false;
                break;
            case 3:
                mode = to_mode_uint(subtoken);
                break;
            case 4:
                vm_pid = atoi(subtoken);
                break;
            default:
                break;
        }
        if(status == 0)
            break;
    }
    if(i != 4)
        return false;

    // TODO: judge qemu process alive
    flags = LKF_PERSISTENT |  LKF_ORPHAN;

    ret = dlm_ls_lockx(lockspace, mode, &lksb, flags,
            name, strlen(name), 0,
            (void *)1, (void *)1, (void *)1,
            NULL, NULL);
    if(ret == 0){
        fprintf(stdout, "adopt lock success --> name: %s, lkid: %d, status: %d\n", name, lksb.sb_lkid, lksb.sb_status);
    }
    else if(ret == -1 && errno == -EAGAIN){
        fprintf(stderr, "adopt lock failed --> name: %s, reason: %s, try again\n", name, strerror(errno));
        return false;
    }
    else if (ret < 0) {
        fprintf(stderr, "adopt lock failed --> name: %s, reason: %s, try again\n", name, strerror(errno));
        return false;
    }

    // add in lk_list
    lk = create_lock(&lk_list);
    if(lk != NULL){
        strncpy(lk->name, name, SHA256LEN-1);
        lk->lkid = lksb.sb_lkid;
        lk->mode = mode;
        lk->vm_pid = vm_pid;
    }

    return true;
}

bool prepare_dlm_lockfile(void){
    int ret, line;
    FILE *fp;
    char *buf = NULL;
    size_t n = 0;
    ssize_t count;
    pid_t previous;
    uint32_t nodeid;

    ret = access(DLM_FILE_PATH, F_OK);
    if(ret == 0){ // file exist
        fp = fopen(DLM_FILE_PATH, "r");
        if(fp == NULL){ // can't read or write
            fprintf(stderr, "%s: %s\n", __func__, strerror(errno));
            return false;
        }
        for(line=1; !feof(fp); line++){
            count = getline(&buf, &n, fp);
            if(count <= 0)
                break;
            if(line == 1){ // previous pid
                previous = atoi(buf);
            }
            else if(line > 2){
                adopt_lock(buf);
            }
        }
        free(buf);

        if(get_local_nodeid(&nodeid)){
            // purge lock
            dlm_ls_purge(lockspace, nodeid, previous);
        }
    }

    return true;
}

bool write_lock(struct lock *lk){
    int fd;
    char buf[BUFSIZ] = {0};

    fd = open(DLM_FILE_PATH, O_WRONLY|O_APPEND);
    if(fd < 0){
        fprintf(stderr, "%s: %s\n", __func__, strerror(errno));
        return false;
    }
    snprintf(buf, BUFSIZ-1, "%6d %32s %10d %10s %10d\n", 1, lk->name, lk->lkid, to_mode_text(lk->mode), lk->vm_pid);
    write(fd, buf, strlen(buf));

    fdatasync(fd);

    close(fd);
    return true;
}

bool write_unlock(uint32_t lkid){
    int fd;
    char buf[BUFSIZ] = {0};
    off_t offset;

    fd = open(DLM_FILE_PATH, O_RDWR);
    if(fd < 0){
        fprintf(stderr, "%s: %s\n", __func__, strerror(errno));
        return false;
    }
    // calculate offset
    offset = 11 + (6+32+10+10+10+5)*lkid;
    lseek(fd, offset, SEEK_SET);
    snprintf(buf, BUFSIZ-1, "%6d", 0);
    write(fd, buf, strlen(buf));
    fdatasync(fd);

    close(fd);
    return true;
    
}

bool initialize_dlm_lockfile(void){
    int fd;
    char buf[BUFSIZ] = {0};
    struct lock *lk;

    fd = open(DLM_FILE_PATH, O_WRONLY|O_CREAT|O_TRUNC);
    if(fd < 0){
        fprintf(stderr, "%s: %s\n", __func__, strerror(errno));
        return false;
    }
    snprintf(buf, BUFSIZ-1, "%10d\n%6s %32s %10s %10s %10s\n", getpid(), STATUS, RESOURCE_NAME, LOCK_ID, LOCK_MODE, QEMU_PID); 
    write(fd, buf, strlen(buf));
    if(!list_empty(&lk_list)){
        list_for_each_entry(lk, &lk_list, entry){
            snprintf(buf, BUFSIZ-1, "%6d %32s %10d %10s %10d\n", 1, lk->name, lk->lkid, to_mode_text(lk->mode), lk->vm_pid);
            write(fd, buf, strlen(buf));
        }
    }

    fdatasync(fd);
    close(fd);

    return 0;
}

bool setup_dlm_lockfile(void){
    int ret;

    if(!lockspace)
        return false;

    // prepare to setup 'dlmlock' file
    if(!prepare_dlm_lockfile())
        return false;

    // initialize 'dlmlock' file
    if(!initialize_dlm_lockfile())
        return false;

    return true;
}
/*
struct lock *create_lock(struct list_head *lk_list){
    struct lock *lk = NULL;

    lk = malloc(sizeof(struct lock));
    if(lk == NULL){
        fprintf(stderr, "%s: %s\n", __func__, strerror(errno));
    }

    list_add_tail(&lk->entry, lk_list);

    return lk;
}
*/
void free_lock(struct lock *lk){
    list_del(&lk->entry);

    free(lk);
}
