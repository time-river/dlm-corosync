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

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <libdlm.h>
#include <corosync/cpg.h>
#include "list.h"

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

#define PRMODE              "PRMODE"
#define EXMODE              "EXMODE"

#define DEBUG_LOG(output, log, ...) \
    do { \
        fprintf(output, "line --> %d func --> %s: " #log "\n", \
                __LINE__, __func__, ##__VA_ARGS__); \
    } while(0)

struct lock {
    struct list_head entry;
    char *name;
    uint32_t mode;
    uint32_t lkid;
    pid_t vm_pid;
};

static struct list_head lk_list;
static dlm_lshandle_t ls;

char *to_mode_text(uint32_t mode);
uint32_t to_mode_uint(const char *mode);
int dlm_running(void);
int setup_lockspace(void);
int setup_lockfile(void);
int write_lock(const struct lock *lk, int fd, int status);
void record_lock(const char *name, const uint32_t mode, const uint32_t lkid, const pid_t vm_pid);
void delete_lock(struct lock *lk);

off_t calculate_offset(uint32_t id);

int main(int argc, char *argv[]){
    list_head_init(&lk_list);

    if(geteuid() != 0){
        DEBUG_LOG(stderr, "need superuser privileges");
        return 0;
    }

    if(dlm_running()){
        fprintf(stdout, "dlm is not running.\n");
        return 0;
    }

    setup_lockspace();
    setup_lockfile();

    int raw_name, raw_mode, op;
    pid_t vm_pid = 121;
    char *name;
    uint32_t mode, flags, lkid;
    struct dlm_lksb lksb;
    int ret, line;
    struct lock *lk;
    int fd;

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
                DEBUG_LOG(stdout, "action --> lock name --> %s", name);
                memset(&lksb, 0, sizeof(lksb));
                ret = dlm_ls_lock_wait(ls, mode, &lksb, LKF_PERSISTENT|LKF_NOQUEUE, name, strlen(name), 0, NULL, NULL, NULL);
                if(ret != 0 || lksb.sb_status != 0){
                    DEBUG_LOG(stderr, "%s\n", strerror(errno));
                }
                else{
                    DEBUG_LOG(stdout, "result --> lock success\n lkid --> %d, status --> %d", lksb.sb_lkid, lksb.sb_status);
                    record_lock(name, mode, lksb.sb_lkid, vm_pid);
                    fd = open(DLM_FILE_PATH, O_RDWR|O_APPEND);
                    write_lock(list_last_entry(&lk_list, struct lock, entry), fd, 1);
                    close(fd);
                }
                break;
            default:
                fprintf(stdout, "action --> unlock\n");
                int fd = open(DLM_FILE_PATH, O_RDWR);
                list_for_each_entry(lk, &lk_list, entry){
                    if((!strcmp(lk->name, name)) && (lk->lkid == lkid)){
                        ret = dlm_ls_unlock_wait(ls, lkid, 0, &lksb);
                        if(!ret || !lksb.sb_status){
                            fprintf(stderr, "dlm_ls_unlock_wait: %s, name: %s lkid: %d, status: %d\n", strerror(errno), name, lkid, lksb.sb_status);
                        }
                        else{
                            fprintf(stdout, "unlock success, name --> %s, lkid --> %d, status: %d\n", name, lkid, lksb.sb_status);
                            delete_lock(lk);
                        }
                        break;
                    }
                }
                break;
        }
    }

    return 0;
}

/*
 * return: 0 running
 *         1 not running
 */
int dlm_running(void){
    int ret;

    ret = access(DLM_CLUSTER_NAME_PATH, F_OK);
    if(ret < 0){
        DEBUG_LOG(stderr, "%s", strerror(errno));
    }

    return ret;
}

int setup_lockspace(void){

    ls = dlm_open_lockspace(LOCKSPACE_NAME);
    if(!ls)
        ls = dlm_create_lockspace(LOCKSPACE_NAME, LOCKSPACE_MODE);
    if(!ls)
        return -1;

    if(dlm_ls_pthread_init(ls)){
        DEBUG_LOG(stderr, "%s", strerror(errno));
        if(errno != -EEXIST)
            return -1;
    }

    return 0;
}

char *to_mode_text(uint32_t mode){
    switch(mode){
        case LKM_PRMODE:
            return PRMODE;
        case LKM_EXMODE:
            return EXMODE;
        default:
            return NULL;
    }
}

uint32_t to_mode_uint(const char *mode){
    if(!strcmp(mode, PRMODE))
        return LKM_PRMODE;
    if(!strcmp(mode, EXMODE))
        return LKM_EXMODE;

    return 0;
}

int get_local_nodeid(unsigned int *nodeid){
    cpg_handle_t handle;
    cs_error_t result;

    result = cpg_model_initialize(&handle, CPG_MODEL_V1, NULL, NULL);
    if(result != CS_OK){
        DEBUG_LOG(stderr, "%s", strerror(errno));
        return -1;
    }
    result = cpg_local_get(handle, nodeid);
    if(result != CS_OK){
        DEBUG_LOG(stderr, "%s", strerror(errno));
        return -1;
    }

    DEBUG_LOG(stdout, "nodeid --> %d", *nodeid);

    result = cpg_finalize(handle);
    if(result != CS_OK){
        DEBUG_LOG(stderr, "%s", strerror(errno));
    }

    return 0;
}

/*
 * STATUS RESOURCE_NAME LOCK_MODE QEMU_PID
 */
int adopt_lock(char *raw){
    char *str, *subtoken, *saveptr;
    int i, status;
    char *name = NULL;
    uint32_t mode;
    struct dlm_lksb lksb = {0};
    pid_t vm_pid;

    for(i=0, str=raw, status=0; ; str=NULL, i++){
        subtoken = strtok_r(str, " \n", &saveptr);
        if(subtoken == NULL)
            break;

        switch(i){
            case 0:
                status = atoi(subtoken);
                break;
            case 1:
                name = strdup(subtoken);
                if(!name)
                    status = 0;
                break;
            case 2:
                mode = to_mode_uint(subtoken);
                if(!mode)
                    status = 0;
                break;
            case 3:
                vm_pid = atoi(subtoken);
                if(!vm_pid)
                    status = 0;
                break;
            default:
                status = 0;
                break;
        }

        if(!status)
            break;
    }

    if(i != 5 || status == 0)
        goto out;

    // TODO: check process

again:
    status = dlm_ls_lockx(ls, mode, &lksb, LKF_PERSISTENT|LKF_ORPHAN,
            name, strlen(name), 0,
            (void *)1, (void *)1, (void *)1,
            NULL, NULL);
    if(!status)
        DEBUG_LOG(stdout, "adopt lock success --> name: %s, lkid: %d, status: %d\n", name, lksb.sb_lkid, lksb.sb_status);
    else if(status && errno == -EAGAIN){
        DEBUG_LOG(stderr, "adopt lock failed --> name: %s, reason: %s, try again\n", name, strerror(errno));
        goto again;
    }
    else{
        DEBUG_LOG(stderr, "adopt lock failed --> name: %s, lkid: %d, reason: %s\n", name, lksb.sb_lkid, strerror(errno));
        return -1;
    }

    record_lock(name, mode, lksb.sb_lkid, vm_pid);

out:
    if(name)
        free(name);

    return 0;
}

int init_empty_lockfile(void){
    int fd;
    char buf[BUFSIZ] = {0};

    fd = open(DLM_FILE_PATH, O_WRONLY|O_CREAT|O_TRUNC);
    if(fd < 0){
        DEBUG_LOG(stderr, "%s", strerror(errno));
        return -1;
    }

    snprintf(buf, BUFSIZ-1, "%10d\n%6s %32s %9s %10s", \
            getpid(), STATUS, RESOURCE_NAME, LOCK_MODE, QEMU_PID);
    write(fd, buf, strlen(buf));
    close(fd);

    return 0;
}

int prep_lk_list(void){
    FILE *fp;
    pid_t previous;
    int line;
    ssize_t count;
    size_t n;
    char *buf;
    uint32_t nodeid;
    int ret = -1;
  
    if(!access(DLM_FILE_PATH, F_OK)){
        fp = fopen(DLM_FILE_PATH, "r");
        if(!fp){
            DEBUG_LOG(stderr, "%s", strerror(errno));
            return -1;
        }

        for(line=0; !feof(fp); line++){
            count = getline(&buf, &n, fp);
            if(count <= 0)
                break;
            switch(line){
            case 0:
                previous = atoi(buf);
                if(!previous)
                    goto out;
                break;
            case 1:
                break;
            default:
                adopt_lock(buf);
                break;
            }
        }
        free(buf);
        if(!get_local_nodeid(&nodeid))
            dlm_ls_purge(ls, nodeid, previous);
    }

    ret = 0;
out: 
    return ret;
}

int dump_lk_list(void){
    int fd;
    struct lock *lk;

    fd = open(DLM_FILE_PATH, O_WRONLY|O_CREAT|O_TRUNC);
    if(fd < 0){
        DEBUG_LOG(stderr, "%s", strerror(errno));
        return -1;
    }

    dprintf(fd, "%10d\n%6s %32s %9s %10s", \
              getpid(), STATUS, RESOURCE_NAME, LOCK_MODE, QEMU_PID);
    list_for_each_entry(lk, &lk_list, entry){
        write_lock(lk, fd, 1);
    }

    close(fd);
    return 0;
}

int setup_lockfile(void){
    
    if(!ls){
        DEBUG_LOG(stderr, "not open lockspace");
        return -1;
    }

    if(prep_lk_list())
        return -1;

    if(dump_lk_list())
        return -1;

    return 0;
}

int write_lock(const struct lock *lk, int fd, int status){
    char buf[BUFSIZ] = {0};
    off_t offset, ret;

    /* <pid>\n
     *    10 1
     * STATUS LOCK_ID RESOURCE_NAME LOCK_MODE QEMU_PID
     *      6      10            32         9       10        
     */
    offset = 10 + 1 + (6+1+10+1+32+1+9+1+10+1) * lk->lkid;
    ret = lseek(fd, offset, SEEK_SET);
    if(ret == -1){
        DEBUG_LOG(stderr, "%s", strerror(errno));
        return -1;
    }
    snprintf(buf, BUFSIZ-1, "%6d %32s %9s %10d\n", \
            status, lk->name, to_mode_text(lk->mode), lk->vm_pid);
    DEBUG_LOG(stdout, "%s", buf);
    write(fd, buf, strlen(buf));
    fdatasync(fd);

    return 0;
}

void record_lock(const char *name, const uint32_t mode, const uint32_t lkid, const pid_t vm_pid){
    struct lock *lk;
    int fd;

    lk = malloc(sizeof(struct lock));
    if(!lk){
        DEBUG_LOG(stderr, "%s", strerror(errno));
        return;
    }

    lk->name = strdup(name);
    lk->mode = mode;
    lk->lkid = lkid;
    lk->vm_pid = vm_pid;

    list_add_tail(&lk->entry, &lk_list);

    return;
}

void delete_lock(struct lock *lk){
    int fd;

    list_del(&lk->entry);

    fd = open(DLM_FILE_PATH, O_RDWR);
    if(fd < 0){
        DEBUG_LOG(stderr, "%s", strerror(errno));    
    }
    write_lock(lk, fd, 0);

    free(lk->name);
    free(lk);

    return;
}

off_t calculate_offset(uint32_t id){
    off_t offset;
    
    offset = 10 + 1 + (6+1+10+1+32+1+9+1+10+1) * id;

    return offset;
}
