virLockManagerSanlockAcquire
    if (priv->vm_pid == getpid()) {

priv->vm_pid 是 vm 的 pid，相关代码:

qemuProcessLaunch
    static int qemuProcessHook(void *data) // fork 之后, execv 之前执行
        h->vm->pid = getpid();

    if (rv == 0) {
        if (virPidFileReadPath(priv->pidfile, &vm->pid) < 0) { // libvirtd 记录 pid

static int virLockManagerSanlockAcquire(virLockManagerPtr lock,
                                        const char *state,
                                        unsigned int flags,
                                        virDomainLockFailureAction action,
                                        int *fd)
     XML element: on_lockfailure for action
