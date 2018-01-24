Relationship:
    DLM is a symmetric general-purpose distributed lock manager, consisting of a kernel
    modules named 'dlm.ko', userspace application named 'libdlm', and cluster
    infrastructure environment which could be 'corosync' responsible for communication.

Characteristic:
    In order to use dlm in cluster, daemons, 'dlm_controld' and 'corosync', must be
    runned before. Because dlm is asynchronous, there is something receiving notification
    from kernel. 'libdlm' provides `dlm_pthread_init` and `dlm_ls_pthread_init` for this
    purpose.

    There is lockspace concept. One lock must belong to one lockspace, and is associated
    with one lock resource, owned by one process. Lock/unlock need specific flag. Lock
    flags `LKF_PERSISTENT` specifies a lock that will not be unlocked when the process
    exists or lockspace closes, but it will become an orphan lock once process disappears.
    New process want to adopt orphan lock, just specific `LKF_ORPHAN` when re-lock using
    the right lock-name and lock-mode.
    
    This means: `LKF_PERSISTENT` lock won't disappear after restarting process, and would
    be inherited by new process. However, restart 'dlm_controld' will result in the lock
    broken, and the action of closing lockspace will lead to lose all lock belong to that
    lockspace.

    Lock could be only released by self-process applied using lkid.

    lkid(lock id) is a mark uniquely identifing a lock in a process, which is returned
    after locking sucessfully or adopting sucessfully.
    
Details:
    lock:
         
