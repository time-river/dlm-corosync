#define _REENTRANT

#include <errno.h>
#include <stdint.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libdlm.h>
#include <corosync/cpg.h>

#define DEBUG_LOG(output, log, ...) \
    do { \
        fprintf(output, "line --> %d func --> %s: " #log "\n", \
                __LINE__, __func__, ##__VA_ARGS__); \
    } while(0)

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

int main(int argc, char *argv[]){
    unsigned int nodeid;
    dlm_lshandle_t ls = NULL;
    int previous = atoi(argv[1]);

    get_local_nodeid(&nodeid);

    ls = dlm_open_lockspace("libvirt");

    DEBUG_LOG(stdout, "privious --> %d nodeid --> %d", previous, nodeid);
    DEBUG_LOG(stdout, "dlm_ls_purge return value --> %d", dlm_ls_purge(ls, nodeid, previous));

    return 0;
}
