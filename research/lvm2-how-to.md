introduce how lvmlockd adopts the lock.

```
struct action {
    struct list_head list;
    uint32_t client_id;
    uint32_t flags;         /* LD_AF_ */
    uint32_t version;
    uint64_t host_id;
    int8_t op;          /* operation type LD_OP_ */
    int8_t rt;          /* resource type LD_RT_ */
    int8_t mode;            /* lock mode LD_LK_ */
    int8_t lm_type;         /* lock manager: LM_DLM, LM_SANLOCK */
    int retries;
    int max_retries;
    int result;
    int lm_rv;          /* return value from lm_ function */
    char vg_uuid[64];
    char vg_name[MAX_NAME+1];
    char lv_name[MAX_NAME+1];
    char lv_uuid[MAX_NAME+1];
    char vg_args[MAX_ARGS+1];
    char lv_args[MAX_ARGS+1];
    char vg_sysid[MAX_NAME+1];
};
```

1. get list of lockspaces from lock managers

dlm running
  --> get all the name of lockspace in list

2. get list of VGs from lvmetad with a lockd type(dlm | sanlock)

```
struct lockspace {
    struct list_head list;      /* lockspaces */
    char name[MAX_NAME+1];
+   char vg_name[MAX_NAME+1];
+   char vg_uuid[64];
+   char vg_args[MAX_ARGS+1];   /* lock manager specific args */
    char vg_sysid[MAX_NAME+1];
+   int8_t lm_type;         /* lock manager: LM_DLM, LM_SANLOCK */
    void *lm_data;
    uint64_t host_id;
    uint64_t free_lock_offset;  /* start search for free lock here */
    
    uint32_t start_client_id;   /* client_id that started the lockspace */
    pthread_t thread;       /* makes synchronous lock requests */
    pthread_cond_t cond;
    pthread_mutex_t mutex;
    unsigned int create_fail : 1;
    unsigned int create_done : 1;
    unsigned int thread_work : 1;
    unsigned int thread_stop : 1;
    unsigned int thread_done : 1;
    unsigned int sanlock_gl_enabled: 1;
    unsigned int sanlock_gl_dup: 1;
    unsigned int free_vg: 1;
    unsigned int kill_vg: 1;
    unsigned int drop_vg: 1;

    struct list_head actions;   /* new client actions */
-   struct list_head resources; /* resource/lock state for gl/vg/lv */
};

struct resource {
    struct list_head list;      /* lockspace.resources */
    char name[MAX_NAME+1];      /* vg name or lv name */
+   int8_t type;            /* resource type LD_RT_ */
    int8_t mode;
    unsigned int sh_count;      /* number of sh locks on locks list */
    uint32_t version;
    uint32_t last_client_id;    /* last client_id to lock or unlock resource */
    unsigned int lm_init : 1;   /* lm_data is initialized */
    unsigned int adopt : 1;     /* temp flag in remove_inactive_lvs */
    unsigned int version_zero_valid : 1;
+   unsigned int use_vb : 1;
    struct list_head locks;
    struct list_head actions;
    char lv_args[MAX_ARGS+1];
    char lm_data[0];        /* lock manager specific data */
};
```

get all VGs with lockd type
  --> for each VGs, create a struct resource on ls->resources

3. get list of active lockd type LVs from /dev

remove inactive LV
  --> compare with something, get the LV status(active or inactive)


?. create and queue start actions to add locksapces

```
        act->op = LD_OP_START;
        act->flags = (LD_AF_ADOPT | LD_AF_WAIT);
        act->rt = LD_RT_GL;
        act->lm_type = LD_LM_DLM;
        act->client_id = INTERNAL_CLIENT_ID;

ls->
    name
    lm_type
    start_client_id
    vg_uuid
    vg_name
    vg_args
    host_id

r->
    type = LD_RT_VG;
    mode = LD_LK_UN;
    use_vb = 1;
    strncpy(r->name, R_NAME_VG, MAX_NAME);

lockspace_thread_main
```

>+1. lock-adopt actions for active LVs and GL/VG locks

```
adopt orphan LV lock

            act->op = LD_OP_LOCK;
            act->rt = LD_RT_LV;
            act->mode = LD_LK_EX;
            act->flags = (LD_AF_ADOPT | LD_AF_PERSISTENT);
            act->client_id = INTERNAL_CLIENT_ID;
            act->lm_type = ls->lm_type;
            strncpy(act->vg_name, ls->vg_name, MAX_NAME);
            strncpy(act->lv_uuid, r->name, MAX_NAME);
            strncpy(act->lv_args, r->lv_args, MAX_ARGS);


adopt orphan VG lock

        act->op = LD_OP_LOCK;
        act->rt = LD_RT_VG;
        act->mode = LD_LK_SH;
        act->flags = LD_AF_ADOPT;
        act->client_id = INTERNAL_CLIENT_ID;
        act->lm_type = ls->lm_type;
        strncpy(act->vg_name, ls->vg_name, MAX_NAME);

adopt orphan GL lock

    act->op = LD_OP_LOCK;
    act->rt = LD_RT_GL;
    act->mode = LD_LK_SH;
    act->flags = LD_AF_ADOPT;
    act->client_id = INTERNAL_CLIENT_ID;
    act->lm_type = (gl_use_sanlock ? LD_LM_SANLOCK : LD_LM_DLM);

        add_dlm_global_lockspace(act);

            act->flags |= LD_AF_SEARCH_LS;
            act->flags |= LD_AF_WAIT_STARTING;
```
