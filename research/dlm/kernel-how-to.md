/*
 * device_write
 *
 *   device_user_lock
 *     dlm_user_request -> request_lock
 *     dlm_user_convert -> convert_lock
 *
 *   device_user_unlock
 *     dlm_user_unlock -> unlock_lock
 *     dlm_user_cancel -> cancel_lock
 *
 *   device_create_lockspace
 *     dlm_new_lockspace
 *
 *   device_remove_lockspace
 *     dlm_release_lockspace
 */

    req->cmd = DLM_USER_LOCK;
    req->i.lock.mode = mode;
    req->i.lock.flags = (flags & ~LKF_WAIT);
    req->i.lock.lkid = lksb->sb_lkid;
    req->i.lock.parent = parent;
    req->i.lock.lksb = lksb;
    req->i.lock.castaddr = astaddr;
    req->i.lock.bastaddr = bastaddr;
    req->i.lock.castparam = astarg; /* same comp and blocking ast arg */
    req->i.lock.bastparam = astarg;

    if (xid) // arg: range
        req->i.lock.xid = *xid;
    if (timeout)
        req->i.lock.timeout = *timeout;

    if (flags & LKF_CONVERT) {
        req->i.lock.namelen = 0;
    } else {
        if (namelen > DLM_RESNAME_MAXLEN) {
            errno = EINVAL;
            return -1;
        }
        req->i.lock.namelen = namelen;
        memcpy(req->i.lock.name, name, namelen);
    }

    if (flags & LKF_VALBLK) {
        memcpy(req->i.lock.lvb, lksb->sb_lvbptr, DLM_LVB_LEN);
    }

    len = sizeof(struct dlm_write_request) + namelen;
    lksb->sb_status = EINPROG;

    if (flags & LKF_WAIT)
        status = sync_write_v6(lsinfo, req, len);
    else
        status = write(lsinfo->fd, req, len);

/* struct passed to the lock write */
struct dlm_lock_params {
    __u8 mode;
    __u8 namelen;
    __u16 unused;
    __u32 flags;
    __u32 lkid;
    __u32 parent;
    __u64 xid;
    __u64 timeout;
    void __user *castparam;
    void __user *castaddr;
    void __user *bastparam;
    void __user *bastaddr;
    struct dlm_lksb __user *lksb;
    char lvb[DLM_USER_LVB_LEN];
    char name[0];
};

/* locks list is kept so we can remove all a process's locks when it
   exits (or orphan those that are persistent) */

struct dlm_user_proc {
    dlm_lockspace_t     *lockspace;
    unsigned long       flags; /* DLM_PROC_FLAGS */
    struct list_head    asts;
    spinlock_t      asts_spin;
    struct list_head    locks;
    spinlock_t      locks_spin;
    struct list_head    unlocking;
    wait_queue_head_t   wait;
};

    ua->proc = proc;
    ua->user_lksb = params->lksb;
    ua->castparam = params->castparam;
    ua->castaddr = params->castaddr;
    ua->bastparam = params->bastparam;
    ua->bastaddr = params->bastaddr;
    ua->xid = params->xid;

