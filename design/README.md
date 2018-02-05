This is the implementation of the RFC from
  https://www.redhat.com/archives/libvir-list/2017-December/msg00689.html

The following is the design of this iplementation.

Environmental requirements:

  DLM is a symmetric general-purpose distributed lock manager,
  consisting of a kernel modules named 'dlm.ko', userspace
  application named 'libdlm', and cluster infrastructure
  environment which could be 'corosync' responsible for communication.

  For more usage about libdlm and corosync,  `man corosync.conf`
  and `man dlm_controld`

Implementation details:

  In order to use dlm in cluster, daemons, 'dlm_controld' and
  'corosync', must have been runned before. Because dlm is
  asynchronous, there is something receiving notification from
  kernel. 'libdlm' provides `dlm_pthread_init` and
  `dlm_ls_pthread_init` for this purpose.

  Here is lockspace concept in dlm. Lockspace is different
  from sanlock, it's just to provide a private namespaceit's
  one namespace for locks that are part of a single application.
  One lock must belong to one lockspace, and is associated with
  one lock resource, owned by one process in one node.
  
  Lock/unlock need specific flag. Lock flags `LKF_PERSISTENT`
  specifies a lock that wouldn't be unlocked if the process was
  dead or lockspace was closed (`dlm_close_lockspace`). lockspace
  is cluster-wide, lock with `LKF_PERSISTENT` flag won't be
  automatically released except that the owner node reboot, but
  it will become an orphan lock once process disappears. New
  process in the same node want to adopt orphan lock, just
  specific `LKF_ORPHAN` when re-lock using the right lock-name
  and lock-mode, new lock id would be assigned for adopted lock.

  lkid(lock id) is a mark uniquely identifing a lock in a process,
  which is returned after locking sucessfully or adopting sucessfully.
  Lock could be only released by self-process applied using lkid.

  The following means: locks won't be automatically released when
  the process(libvirt) that acquired them dies, daemon likes
  virtlockd isn't necessary. In order to adopt orphan locks, there
  must be some lock information left after the process died. A
  file would store the lock information in this design.

  However, I find that the existence of "leaked" fd in vm instance
  process inherited from libvirt would tell lock daemon to release
  their locks instead of libvirt. So destroy or shutdown qemu
  instance won't make dlm release locks. I refer to the action of
  libxl(`libxlDomainCleanup` function in 'src/libxl/libxl_domain.c'),
  add some code after unlink pid and xml files in `qemuProcessStop`
  ('src/qemu/qemu_process.c') to proactively tell lock manager
  release locks.

  Adhere to the simple, enough principle, lock information using
  by running libvirt process is stored in list instead of hash
  table.
