# lock manager 实现

## 实现

virDomainLockManagerNew(plugin, uri, dom, withResources, flags)
 -- lock = virLockManagerNew(plugin->driver, VIR_LOCK_MANAGER_OBJECT_TYPE_DOMAIN, nparams, params, flags)
 -- if (withResources)
        virDomainLockManagerAddLease(lock, dom->def->leases[i]) // Adding leases
        virDomainLockManagerAddImage(lock, disk->src) // Adding disks
 -- return lock

virDomainLockManagerAddImage(lock, src)
 -- virLockManagerAddResource(lock, VIR_LOCK_MANAGER_RESOURCE_TYPE_DISK, src->path, 0, NULL, diskFlags) 
        // diskFalgs: readonly(VIR_LOCK_MANAGER_RESOURCE_READONLY), shared(VIR_LOCK_MANAGER_RESOURCE_SHARED)

virDomainLockManagerAddLease(lock, lease)
 -- virLockManagerAddResource(lock, VIR_LOCK_MANAGER_RESOURCE_TYPE_LEASE, lease->key, nparams, lparams, leaseFlags)

/**
 * @driver: the lock manager implementation to use
 * @type: the type of process to be supervised
 * @flags: bitwise-OR of virLockManagerNewFlags
 *
 * @name: disk absolute path
**/

virLockManagerNew(driver, type, nparams, params, flags)
 -- lock->driver->drvNew(lock, type, nparams, params, flags)

virLockManagerAddResource(lock, type, name, nparams, params, flags)
 -- lock->driver->drvAddResource(lock, type, name, nparams, params, flags)

virLockManagerAcquire(lock, state, flags, action, fd)
 -- lock->driver->drvAcquire(lock, state, flags, action, fd)

virLockManagerRelease(lock, state, flags)
 -- lock->driver->drvRelease(lock, state, flags)

virLockManagerInquire(lock, state, flags)
 -- lock->driver->drvInquire(lock, state, flags)

virLockManagerFree(lock)
 -- lock->driver->drvFree(lock)


## 场景

virDomainLockProcessStart
virDomainLockProcessPause
virDomainLockProcessResume
virDomainLockProcessInquire

#### 流程

/*
 * @dom: vm information
 */

qemuProcessStart
 -- qemuProcessLaunch
     -- qemuProcessHook
         -- virDomainLockProcessStart
             -- lock = virDomainLockManagerNew(plugin, uri, dom, true, VIR_LOCK_MANAGER_NEW_STARTED)
             -- virLockManagerAcquire(lock, NULL, flags, dom->def->onLockFailure, fd)
                    // flags = VIR_LOCK_MANAGER_ACQUIRE_RESTRICT | VIR_LOCK_MANAGER_ACQUIRE_REGISTER_ONLY
             -- virLockManagerFree(lock)
     -- virCommandRun // execv || execvp

qemuProcessHandleStop // | qemuProcessHandleWatchdog | qemuProcessHandleIOError
 -- virDomainLockProcessPause
     -- lock = virDomainLockManagerNew(plugin, NULL, dom, true, 0)
     -- virLockManagerRelease(lock, state, 0)
     -- virLockManagerFree(lock)
 -- virDomainSaveStatus

qemuProcessStartCPUs // Precondition: vm must be locked, and a job must be active.
 -- virDomainLockProcessResume // prestart
     -- lock = virDomainLockManagerNew(plugin, uri, dom, true, 0)
     -- virLockManagerAcquire(lock, state, 0, dom->def->onLockFailure, NULL)
     -- virLockManagerFree(lock)
 -- qemuMonitorStartCPUs // start

qemuMigrationBegin
 -- qemuMigrationJobStart
 -- qemuMigrationBeginPhase // 迁移过程中出现, 检查各种热插拔设备的状态，并假设存在lock，启动迁移
     -- qemuMigrationBakeCookie
         -- qemuMigrationCookieAddLockstate
             -- virDomainLockProcessInquire // VM 的状态必须为live
                 -- lock = virDomainLockManagerNew(plugin, NULL, dom, true, 0)
                 -- virLockManagerInquire(lock, state, 0)
                 -- virLockManagerFree(lock)
 -- qemuMigrationJobFinish

## disk
virDomainLockDiskAttach
virDomainLockDiskDetach

#### 流程

热插拔资源加锁

qemuDomainAttachDeviceDiskLive
 -- case VIR_DOMAIN_DISK_DEVICE_FLOPPY:
        qemuDomainChangeEjectableMedia
         -- qemuHotplugPrepareDiskAccess
             -- virDomainLockDiskAttach
                 -- virDomainLockImageAttach(plugin, uri, dom, disk->src)
                     -- lock = virDomainLockManagerNew(plugin, uri, dom, false, 0)
                     -- virDomainLockManagerAddImage(lock, src)
                     -- virLockManagerAcquire(lock, NULL, 0, dom->def->onLockFailure, NULL)
                     -- virLockManagerFree(lock)
         -- qemuHotplugWaitForTrayEject // attach

qemuDomainRemoveDevice
 -- case VIR_DOMAIN_DEVICE_DISK:
        qemuDomainRemoveDiskDevice(driver, vm, dev->data.disk) // 其他调用此命令的函数: qemuDomainDetachVirtioDiskDevice | qemuDomainDetachDiskDevice
         -- virDomainLockDiskDetach
             -- virDomainLockImageDetach(plugin, dom, disk->src)
                 -- lock = virDomainLockManagerNew(plugin, NULL, dom, false, 0)
                 -- virDomainLockManagerAddImage(lock, src)
                 -- virLockManagerRelease(lock, NULL, 0)
                 -- virLockManagerFree(lock)
         -- virDomainDiskDefFree // remove

## image
virDomainLockImageAttach
virDomainLockImageDetach

#### 实现

qemuDomainDiskChainElementPrepare // Allow a VM access to a single element of a disk backing chain, make lock manager aware of each new file before it is added to a chain
 -- virDomainLockImageAttach
     -- lock = virDomainLockManagerNew(plugin, uri, dom, false, 0)
     -- virDomainLockManagerAddImage(lock, src)
     -- virLockManagerAcquire(lock, NULL, 0, dom->def->onLockFailure, NULL)
     -- virLockManagerFree(lock)

qemuDomainDiskChainElementRevoke // Revoke access to a single backing chain element
 -- virDomainLockImageDetach
     -- lock = virDomainLockManagerNew(plugin, NULL, dom, false, 0)
     -- virDomainLockManagerAddImage(lock, src)
     -- virLockManagerRelease(lock, NULL, 0)
     -- virLockManagerFree(lock)

## lease
virDomainLockLeaseAttach
virDomainLockLeaseDetach

#### 实现

/*
struct _virDomainLeaseDef {
    char *lockspace;
    char *key;
    char *path;
    unsigned long long offset;
};

compare:

virDomainLockManagerAddImage(lock, src)
@src: image information

virDomainLockManagerAddLease(lock, lease)
@lease: hotplug device information
*/

qemuDomainAttachLease
 -- virDomainLockLeaseAttach
     -- lock = virDomainLockManagerNew(plugin, uri, dom, false, 0)
     -- virDomainLockManagerAddLease(lock, lease)
     -- virLockManagerAcquire(lock, NULL, 0, dom->def->onLockFailure, NULL)
     -- virLockManagerFree(lock)

qemuDomainDetachLease
 -- virDomainLockLeaseDetach
     -- lock = virDomainLockManagerNew(plugin, NULL, dom, false, 0)
     -- virDomainLockManagerAddLease(lock, lease)
     -- virLockManagerRelease(lock, NULL, 0)
 -- virLockManagerFree(lock)
