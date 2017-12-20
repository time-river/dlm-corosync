flags:
- LKF_NOQUEUE
- LKF_NODLCKWT
- LKF_PERSISTENT
    - `dlm_purge()`


  If the lock manager cannot grant your lock request, it normally adds your request to the end of the wait queue, along with all other blocked lock requests on the lock resource. However, you can specify that the lock manager not queue your request if it cannot be granted immediately by specifying the LKF_NOQUEUE flag as an argument to the lock routine.
  If your lock request cannot be granted immediately, the dlm_lock routine returns the status 0 and the AST is queued with the status EAGAIN in the status field of the lock status block.

  To exclude a lock request from the lock manager’s deadlock detection processing, specify the LKF_NODLCKWT flag with the dlm_lock routine.

  When a client terminates while holding one or more locks, the lock manager purges any locks that do not have the LKF_PERSISTENT flag set. Locks originally requested with the LKF_PERSISTENT flag set remain after a client terminates. Applications use orphan locks to prevent other lock clients from accessing a lock resource until any clean up made necessary by the termination has been performed. Once the LKF_PERSISTENT flag is set (whether by the initial lock request or by a subsequent conversion), that flag remains set for the duration of that lock.

  The DLM API includes the dlm_purge routine to facilitate releasing locks. The dlm_purge routine releases all locks owned by a particular client, identified by its process ID. When you specify a process ID of 0, all orphaned locks for the specified node ID are released.
