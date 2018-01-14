#include <stdio.h>
#include <corosync/cpg.h>

int main(void){
    cpg_handle_t handle;
    int result;
    unsigned int nodeid;

    result = cpg_model_initialize(&handle, CPG_MODEL_V1, NULL, NULL);
    cpg_local_get(handle, &nodeid);

    printf("nodeid: %u\n", nodeid);

    cpg_finalize(handle);

    return 0;
}
