https://access.redhat.com/errata/RHBA-2014:0256
https://bugzilla.redhat.com/show_bug.cgi?id=1070905


after
  qemuProcessLaunch
    virCommandRun

after `fork()`(two times), before hanshake:
childProcess: <name>.pid file appear -- `virPidFileWritePath // in virExec, vircommand.c +689`, after the first `fork`, exit after write file
  then acquire lock(flag: RESTRICT) -- `cmd->hook(cmd->opaque), vircommand.c +727`, after the second `fork`
parentProcess: <name>.xml file appear -- `virDomainSaveStatus // qemu_process.c +5905`

after hanshake, `execv(qemu)`

destroy, shutdown question:

qemuMonitorIO
 -- (eofNotify)(mon, vm, mon->callbackOpaque) // callback, src/qemu/qemu_monitor.c +775


 virEventPollRemoveHandle
 
qemuProcessStop // src/qemu/qemu_process.c +6029 
 -- qemuProcessKill
 -- qemuDomainCleanupRun
 -- qemuProcessAutoDestroyRemove
     -- virCloseCallbacksUnset
 ??
 -- qemuSecurityRestoreAllLabel // src/qemu/qemu_security.c +66
     -- virSecurityDACRestoreAllLabel
         -- virSecurityDACRestoreFileLabelInternal // src/security/security_dac.c +668
             -- virSecurityDACSetOwnershipInternal

 -- networkReleaseActualDevice // src/network/bridge_driver.c +4985
     -- networkLogAllocation // src/network/bridge_driver.c +4295

 -- qemuRemoveCgroup // src/qemu/qemu_cgroup.c +1138
     -- virCgroupRemove // src/util/vircgroup.c +3507
         -- virCgroupRemoveRecursively
 ...
 -- qemuProcessRemoveDomainStatus(driver, vm); // unlink pid file & xml file
