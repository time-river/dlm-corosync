Relationship:
    DLM is a symmetric general-purpose distributed lock manager, consisting of a kernel
    modules named 'dlm.ko', userspace application named 'libdlm', and cluster
    infrastructure environment which could be 'corosync' responsible for communication.

Characteristic:
    In order to use dlm in cluster, daemons, 'dlm_controld' and 'corosync', must be
    runned before. And one thread is responsible for communication between application
    and ...? 'libdlm' provides `dlm_pthread_init` and `dlm_ls_pthread_init` for this
    purpose.

    Lock is associated with process. Lock/unlock need specific flag. Lock flags
    `LKF_PERSISTENT` specifies a lock that will not be unlocked when the process exists,
    but it will become an orphan lock once process disappears. New process want to
    adopt orphan lock, just specific `LKF_ORPHAN` when re-lock using the right lock-name
    and lock-mode.
    
    Lock could be only released by process applied using lkid.

    lkid(lock id) is a mark uniquely identifing a lock in a process, which is returned
    after locking sucessfully or adopting a sucessfully. 

Lock:
    
