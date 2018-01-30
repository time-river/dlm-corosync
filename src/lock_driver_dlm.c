#include <config.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <stdint.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>

#include <corosync/cpg.h>
#include <libdlm.h>
#include "virlist.h"

#include "lock_driver.h"
#include "viralloc.h"
#include "virconf.h"
#include "vircrypto.h"
#include "virerror.h"
#include "virfile.h"
#include "virlog.h"
#include "virstring.h"
#include "virthread.c"

#define VIR_FROM_THIS VIR_FROM_LOCKING

#define DLM_LOCKSPACE_MODE  0600
#define DLM_LOCKSPACE_NAME  "libvirt"
#define DLM_FILE_PATH       "/tmp/dlmfile"
#define DLM_FILE_MODE       0644

#define PRMODE  "PRMODE"
#define EXMODE  "EXMODE"

#define STATUS             "STATUS"
#define RESOURCE_NAME      "RESOURCE_NAME"
#define LOCK_ID            "LOCK_ID"
#define LOCK_MODE          "LOCK_MODE"
#define VM_PID             "VM_PID"

#define BUFFERLEN          128

/* This will be set after dlm_controld is started. */
#define DLM_CLUSTER_NAME_PATH "/sys/kernel/config/dlm/cluster/cluster_name"

VIR_LOG_INIT("locking.lock_driver_dlm");

typedef struct _virLockInformation virLockInformation;
typedef virLockInformation *virLockInformationPtr;

typedef struct _virLockManagerDlmResource virLockManagerDlmResource;
typedef virLockManagerDlmResource *virLockManagerDlmResourcePtr;

typedef struct _virLockManagerDlmPrivate virLockManagerDlmPrivate;
typedef virLockManagerDlmPrivate *virLockManagerDlmPrivatePtr;

typedef struct _virLockManagerDlmDriver virLockManagerDlmDriver;
typedef virLockManagerDlmDriver *virLockManagerDlmDriverPtr;

struct _virLockInformation {
    struct list_head entry;
    char    *name;
    uint32_t mode;
    uint32_t lkid;
    pid_t   vm_pid;
};

struct _virLockManagerDlmResource {
    char *name;
    uint32_t mode;
};

struct _virLockManagerDlmPrivate {
    unsigned char vm_uuid[VIR_UUID_BUFLEN];
    char *vm_name;
    pid_t vm_pid;
    int   vm_id;
    const char *vm_uri;

    size_t nresources;
    virLockManagerDlmResourcePtr resources;

    bool hasRWDisks;
};

struct _virLockManagerDlmDriver {
    bool autoDiskLease;
    bool requireLeaseForDisks;

	bool adoptLock;
	char *lockspaceName;
    char *dlmFilePath;
};

static virLockManagerDlmDriverPtr driver = NULL;
static dlm_lshandle_t lockspace = NULL;
static virMutex mutex;
static struct list_head lockList;

static int virLockManagerDlmLoadConfig(const char *configFile)
{
    virConfPtr conf = NULL;
    int ret = -1;

    if (!configFile)
        return 0; // it would works well eventhough configfile does not exist.

    if (access(configFile, R_OK) == -1) {
        if (errno != ENOENT) {
            virReportSystemError(errno,
                                 _("Unable to access config file %s"),
                                 configFile);
            return -1;
        }
        return 0;
    }

	if (!(conf = virConfReadFile(configFile, 0)))
		return -1;	

    if (virConfGetValueBool(conf, "auto_disk_leases", &driver->autoDiskLease) < 0)
        goto cleanup;

    if (virConfGetValueBool(conf, "require_lease_for_disks", &driver->requireLeaseForDisks) < 0)
        goto cleanup;

    if (virConfGetValueBool(conf, "adopt_lock", &driver->adoptLock) < 0)
        goto cleanup;

    if (virConfGetValueString(conf, "lockspace_name", &driver->lockspaceName) < 0)
        goto cleanup;

    if (virConfGetValueString(conf, "dlm_file_path", &driver->dlmFilePath) < 0)
        goto cleanup;

    ret = 0;

 cleanup:
    virConfFree(conf);
    return ret;
}

static int virLockManagerDlmToModeUint(const char *token)
{
    if (STREQ(token, PRMODE))
        return LKM_PRMODE;
    if (STREQ(token, EXMODE))
        return LKM_EXMODE;

    return 0;
}

static const char *virLockManagerDlmToModeText(const uint32_t mode)
{
    switch (mode) {
    case LKM_PRMODE:
        return PRMODE;
    case LKM_EXMODE:
        return EXMODE;
    default:
        return NULL;
    }
}

static virLockInformationPtr virLockManagerDlmRecordLock(const char *name,
                                                         const uint32_t mode,
                                                         const uint32_t lkid,
                                                         const pid_t vm_pid)
{
    virLockInformationPtr lock = NULL;

    if (VIR_ALLOC(lock) < 0)
        goto error;

    if (VIR_STRDUP(lock->name, name) < 0)
        goto error;

    lock->mode = mode;
    lock->lkid = lkid;
    lock->vm_pid = vm_pid;

    virMutexLock(&mutex);
    list_add_tail(&lock->entry, &lockList);
    virMutexUnlock(&mutex);

    virReportError(VIR_ERR_INTERNAL_ERROR, _("list_empty=%d"), list_empty(&lockList));
    VIR_DEBUG("record lock sucessfully, lockId: %d", lkid);

    return lock;

 error:
    if (lock)
        VIR_FREE(lock->name);
    return NULL;
}

static void virLockManagerDlmWriteLock(virLockInformationPtr lock, int fd, bool status)
{
    char buffer[BUFFERLEN] = {0};
    off_t offset = 0, rv = 0;

    virReportError(VIR_ERR_INTERNAL_ERROR, _("lockName=%s lockId=%d"), lock->name, lock->lkid);

    if (!lock) {
        virReportError(VIR_ERR_INTERNAL_ERROR, "%s",
                       _("lock is NULL"));
        return;
    }

    /* <pid>\n
     *    10 1
     * STATUS RESOURCE_NAME LOCK_MODE      PID\n
     *      6            32         9       10  
     * 61 = 10 + 1 \
     *     + 6 + 1 +32 + 1 + 9 + 1 + 10 +1
     */
    offset = 11 + 61 * lock->lkid;
	rv = lseek(fd, offset, SEEK_SET);
	if (rv < 0) {
		virReportSystemError(errno, "%s",
							 _("lseek failed"));
        return;
    }
    
    snprintf(buffer, sizeof(buffer), "%6d %32s %9s %10jd\n", \
             status, lock->name,
             NULLSTR(virLockManagerDlmToModeText(lock->mode)),
             (intmax_t)lock->vm_pid);
    virReportError(VIR_ERR_INTERNAL_ERROR, _("write %s length=%zu to fd=%d"), buffer, strlen(buffer), fd);
    if (safewrite(fd, buffer, strlen(buffer)) != 61) {
        virReportSystemError(errno, "%s",
                             _("write lock failed"));
        return;
    }

    VIR_DEBUG("write '%s'", buffer);

    fdatasync(fd);

    return;
}

static void virLockManagerDlmAdoptLock(char *raw) {
    char *str = NULL, *subtoken = NULL, *saveptr = NULL;
    int i = 0, status = 0;
    char *name = NULL;
    uint32_t mode = 0;
    struct dlm_lksb lksb = {0};
    pid_t vm_pid = 0;

    /* STATUS RESOURCE_NAME LOCK_MODE PID */
    for (i = 0, str = raw, status = 0; ; str = NULL, i++) {
        subtoken = strtok_r(str, " \n", &saveptr);
        if (subtoken == NULL)
            break;

        switch(i) {
        case 0:
            status = atoi(subtoken);
            break;
        case 1:
            if (VIR_STRDUP(name, subtoken) != 1)
                status = 0;
            break;
        case 2:
            mode = virLockManagerDlmToModeUint(subtoken);
            if (!mode)
                status = 0;
            break;
        case 3:
            vm_pid = atoi(subtoken);
            if (!vm_pid)
                status = 0;
            break;
        default:
            status = 0;
            break;
        }

        if (!status)
            break;
    }

    if (!status || i != 4)
        goto out;

    status = dlm_ls_lockx(lockspace, mode, &lksb, LKF_PERSISTENT|LKF_ORPHAN,
                          name, strlen(name), 0,
                          (void *)1, (void *)1, (void *)1,
                          NULL, NULL);
    if (status) {
        virReportSystemError(errno,
                             _("adopt lock failed, lockName=%s lockMode=%s"),
                             name, NULLSTR(virLockManagerDlmToModeText(mode)));
        goto out;
    }

    if (!virLockManagerDlmRecordLock(name, mode, lksb.sb_lkid, vm_pid)) {
        // TODO: record adopt failed
    }

    virReportError(VIR_ERR_INTERNAL_ERROR,
                  _("adopt lock, lockName=%s lockId=%d"), name, lksb.sb_lkid);
    return;

 out:
    if (name)
        VIR_FREE(name);

    return;
}

static int virLockManagerDlmGetLocalNodeId(uint32_t *nodeId)
{
    cpg_handle_t handle = 0;

    if (cpg_model_initialize(&handle, CPG_MODEL_V1, NULL, NULL) != CS_OK) {
		// 	TODO
		return -1;
    }
    
	if( cpg_local_get(handle, nodeId) != CS_OK) {
		// TODO
        return -1;
    }

    VIR_DEBUG("nodeid: %d", *nodeId);

    if (cpg_finalize(handle) != CS_OK) {
		// TODO
	}

	return 0;
}

static int virLockManagerDlmPrepareLockfile(const char *dlmFilePath, const bool adoptLock)
{
    FILE *fp = NULL;
    int line = 0;
    size_t n = 0;
    ssize_t count = 0;
    pid_t previous = 0;
    char *buffer = NULL;
    uint32_t nodeId = 0;

    if (!access(dlmFilePath, F_OK)) {
        fp = fopen(dlmFilePath, "r");
        if (!fp) {
            virReportSystemError(errno,
                                 _("open %s failed"), dlmFilePath);
            return -1;
        }

        for (line = 0; !feof(fp); line++) {
            count = getline(&buffer, &n, fp);
            if (count <= 0)
                break;

            switch (line) {
            case 0:
                previous = atoi(buffer);
                if (!previous) {
                    virReportError(VIR_ERR_INTERNAL_ERROR,
                                   _("%s file may be broken, please check and remove it."), dlmFilePath);
                    return -1;
                }
                break;
            case 1:
                break;
            default:
                virLockManagerDlmAdoptLock(buffer);
                break;
            }

            if (!adoptLock)
                break;
        }

        if (!virLockManagerDlmGetLocalNodeId(&nodeId)) {
            if (dlm_ls_purge(lockspace, nodeId, 0)) {
				// TODO
				VIR_DEBUG("dlm_ls_purge error.");
            }
            else {
                virReportError(VIR_ERR_INTERNAL_ERROR,
                               _("dlm_ls_purge success, nodeId=%u previous=%jd"), nodeId, (intmax_t)previous);
            }
        }
        fclose(fp);
        VIR_FREE(buffer);
    }


    return 0;
}

static int virLockManagerDlmDumpLockfile(const char *dlmFilePath)
{
    virLockInformationPtr lock = NULL;
    char buffer[BUFFERLEN] = {0};
    int fd = -1, rv = -1;

    fd = open(dlmFilePath, O_WRONLY|O_CREAT|O_TRUNC, DLM_FILE_MODE);
    if (fd < 0) {
        virReportSystemError(errno, "%s",
                             _("open dlm file failed"));
        return -1;
    }

    snprintf(buffer, sizeof(buffer), "%10jd\n%6s %32s %9s %10s\n", \
             (intmax_t)getpid(), STATUS, RESOURCE_NAME, LOCK_MODE, VM_PID);
    if (safewrite(fd, buffer, strlen(buffer)) != strlen(buffer)) {
        virReportSystemError(errno,
                             _("failed to write file, '%s'"), dlmFilePath);
        goto out;
    }

    list_for_each_entry(lock, &lockList, entry) {
        virLockManagerDlmWriteLock(lock, fd, 1);
    }

    if (VIR_CLOSE(fd) < 0) {
        virReportSystemError(errno,
                             _("Unable to save file %s"),
                             dlmFilePath);
        goto out;
    }

    rv = 0;
 out:
    if (rv)
        VIR_FORCE_CLOSE(fd);
    return rv;
}

static int virLockManagerDlmSetupLockFile(const char *dlmFilePath, const bool newLockspace, const bool adoptLock)
{
    if (!newLockspace &&
        virLockManagerDlmPrepareLockfile(dlmFilePath, adoptLock)) {
        virReportError(VIR_ERR_INTERNAL_ERROR, "%s",
                       _("dlm prepare lockfile failed"));
        return -1;
    }

    if (virLockManagerDlmDumpLockfile(dlmFilePath)) {
        virReportError(VIR_ERR_INTERNAL_ERROR, "%s",
                       _("dlm dump lockfile failed"));
        return -1;
    }

    return 0;
}

static int virLockManagerDlmSetup(void)
{
    bool newLockspace = false;

    list_head_init(&lockList);

    /* check dlm is running */
    if (access(DLM_CLUSTER_NAME_PATH, F_OK)) {
        virReportSystemError(errno, "%s",
                             _("Check dlm_controld, ensure it has setuped"));
        return -1;
    }

    /* open lockspace */
    lockspace = dlm_open_lockspace(driver->lockspaceName);
    if (!lockspace) {
        lockspace = dlm_create_lockspace(driver->lockspaceName, DLM_LOCKSPACE_MODE);
        if (!lockspace) {
            virReportSystemError(errno, "%s",
                                 _("dlm can't open lockspace, and crate failed"));
            return -1;
        }
        newLockspace = true;
    }

    if (dlm_ls_pthread_init(lockspace)) {
        if (errno != EEXIST) {
            virReportSystemError(errno, "%s",
                                 _("dlm thread initialize failed"));
            return -1;
        }
    }

    if (virLockManagerDlmSetupLockFile(driver->dlmFilePath, newLockspace, driver->adoptLock)) {
        virReportError(VIR_ERR_INTERNAL_ERROR, "%s",
                       _("setup dlm lockfile failed"));
        return -1;
    }

    return 0;
}

static int virLockManagerDlmDeinit(void);

static int virLockManagerDlmInit(unsigned int version,
                                 const char *configFile,
                                 unsigned int flags)
{
    VIR_DEBUG("version=%u configFile=%s flags=0x%x", version, NULLSTR(configFile), flags);

    virCheckFlags(0, -1);

    if (geteuid() != 0) {
        virReportError(VIR_ERR_INTERNAL_ERROR, "%s",
                       _("Dlm lock requires superuser privileges"));
        return -1;
    }

    if (driver)
        return 0;

    if (VIR_ALLOC(driver) < 0)
        return -1;

    driver->autoDiskLease = true;
    driver->requireLeaseForDisks = !driver->autoDiskLease;
    driver->adoptLock = true;

    if (VIR_STRDUP(driver->lockspaceName, DLM_LOCKSPACE_NAME) < 0)
        goto cleanup;

    if (VIR_STRDUP(driver->dlmFilePath, DLM_FILE_PATH) < 0)
        goto cleanup;

    if (virLockManagerDlmLoadConfig(configFile) < 0)
        goto cleanup;

    if (virLockManagerDlmSetup() < 0)
        goto cleanup;

    return 0;

 cleanup:
    virLockManagerDlmDeinit();
    return -1;
}

static int virLockManagerDlmDeinit(void)
{
    if (!driver)
        return 0;

    if(lockspace)
        dlm_close_lockspace(lockspace);

    VIR_FREE(driver->lockspaceName);
    VIR_FREE(driver->dlmFilePath);
    VIR_FREE(driver);

    return 0;
}

static int virLockManagerDlmNew(virLockManagerPtr lock,
                                unsigned int type,
                                size_t nparams,
                                virLockManagerParamPtr params,
                                unsigned int flags)
{
    virLockManagerDlmPrivatePtr priv = NULL;
    size_t i;

    virCheckFlags(VIR_LOCK_MANAGER_NEW_STARTED, -1);
    /* flags related:
     *   https://libvirt.org/git/?p=libvirt.git;a=commit;h=da879e592142291709f7b95b9218d32bb1869d1a
     *   https://libvirt.org/git/?p=libvirt.git;a=commit;h=54972be8430c709a338c0716ebc48b02be25dcb7
     */

    if (!driver) {
        virReportError(VIR_ERR_INTERNAL_ERROR, "%s",
                       _("Dlm plugin is not initialized"));
        return -1;
    }

    if (type != VIR_LOCK_MANAGER_OBJECT_TYPE_DOMAIN) {
        virReportError(VIR_ERR_INTERNAL_ERROR,
                       _("Unsupported object type %d"), type);
        return -1;
    }

    if (VIR_ALLOC(priv) < 0)
        return -1;

    for (i = 0; i< nparams; i++) {
        if (STREQ(params[i].key, "uuid")) {
            memcpy(priv->vm_uuid, params[i].value.uuid, VIR_UUID_BUFLEN);
        } else if (STREQ(params[i].key, "name")) {
            if (VIR_STRDUP(priv->vm_name, params[i].value.str) < 0)
                return -1;
        } else if (STREQ(params[i].key, "id")) {
            priv->vm_id = params[i].value.ui;
        } else if (STREQ(params[i].key, "pid")) {
            priv->vm_pid = params[i].value.iv;
        } else if (STREQ(params[i].key, "uri")) {
            /* ignored */
        } else {
            virReportError(VIR_ERR_INTERNAL_ERROR,
                           _("Unexpected parameter %s for object"),
                           params[i].key);
        }
    }

    if (priv->vm_pid == 0)
        VIR_DEBUG("Missing PID parameter for domain object");
    if (!priv->vm_name) {
        virReportError(VIR_ERR_INTERNAL_ERROR, "%s",
                       _("Missing name parameter for domain object"));
        return -1;
    }

    lock->privateData = priv;

    return 0;
}

static void virLockManagerDlmFree(virLockManagerPtr lock)
{
    virLockManagerDlmPrivatePtr priv = lock->privateData;
    size_t i;

    if (!priv)
        return;

    for (i = 0; i < priv->nresources; i++) {
   		VIR_FREE(priv->resources[i].name);
        // TODO ?
    }

    VIR_FREE(priv->resources);
    VIR_FREE(priv->vm_name);
    VIR_FREE(priv);
    lock->privateData = NULL;

    return;
}

static int virLockManagerDlmAddResource(virLockManagerPtr lock,
                                        unsigned int type, const char *name,
                                        size_t nparams,
                                        virLockManagerParamPtr params,
                                        unsigned int flags)
{
    virLockManagerDlmPrivatePtr priv = lock->privateData;
    char *newName = NULL;

    virCheckFlags(VIR_LOCK_MANAGER_RESOURCE_READONLY |
            VIR_LOCK_MANAGER_RESOURCE_SHARED, -1);

    /* Treat read only resources as a no-op lock request */
    if (flags & VIR_LOCK_MANAGER_RESOURCE_READONLY)
        return 0;

    /* don't diff 'VIR_LOCK_MANAGER_RESOURCE_TYPE_DISK' and 'VIR_LOCK_MANAGER_RESOURCE_TYPE_LEASE'
     *  http://ssdxiao.github.io/linux/2017/04/12/Libvirt-Sanlock.html
     *  https://libvirt.org/formatdomain.html#elementsLease
     *  https://libvirt.org/git/?p=libvirt.git;a=commit;h=1ea83207c815e12b1ef60f48a4663e12fbc59687
     *  https://www.redhat.com/archives/libvir-list/2011-May/msg01539.html
     */
    switch (type) {
    case VIR_LOCK_MANAGER_RESOURCE_TYPE_DISK:
        /* https://libvirt.org/git/?p=libvirt.git;a=commit;h=97e4f21782b49f6147a5fc5092740948fc89ede9
         */
            if (params || nparams) {
                virReportError(VIR_ERR_INTERNAL_ERROR, "%s",
                               _("Unexpected parameters for disk resource"));
                return -1;
            }

            if (!driver->autoDiskLease) {
                if (!(flags & (VIR_LOCK_MANAGER_RESOURCE_SHARED |
                               VIR_LOCK_MANAGER_RESOURCE_READONLY))) {
                    priv->hasRWDisks = true;
                    /* ignore disk resource without error */
                    return 0;
                }
            }

            if (virCryptoHashString(VIR_CRYPTO_HASH_MD5, name, &newName) < 0)
                goto cleanup;

        break;

    case VIR_LOCK_MANAGER_RESOURCE_TYPE_LEASE:
        /* TODO: name must less 64 bytes
         * not implement 'offset'
         */
        if (VIR_STRDUP(newName, name) < 0)
            goto cleanup;
        break;

    default:
        virReportError(VIR_ERR_INTERNAL_ERROR,
                _("Unknown lock manager object type %d"),
                type);
        return -1;
    }

    if (VIR_EXPAND_N(priv->resources, priv->nresources, 1) < 0)
        goto cleanup;

    priv->resources[priv->nresources-1].name = newName;

    if (!!(flags & VIR_LOCK_MANAGER_RESOURCE_SHARED))
        priv->resources[priv->nresources-1].mode = LKM_PRMODE;
    else
        priv->resources[priv->nresources-1].mode = LKM_EXMODE;

    return 0;

 cleanup:
    VIR_FREE(newName);

    return -1;
}

static int virLockManagerDlmAcquire(virLockManagerPtr lock,
                                    const char *state ATTRIBUTE_UNUSED,
                                    unsigned int flags,
                                    virDomainLockFailureAction action ATTRIBUTE_UNUSED,
                                    int *fd)
{
    virLockManagerDlmPrivatePtr priv = lock->privateData;
    virLockInformationPtr theLock = NULL;
    struct dlm_lksb lksb = {0};
    int rv = -1, ifd = -1;
    size_t i;

    virCheckFlags(VIR_LOCK_MANAGER_ACQUIRE_REGISTER_ONLY |
                  VIR_LOCK_MANAGER_ACQUIRE_RESTRICT, -1);

    /* requireLeaseForDisks related:
     *  https://libvirt.org/git/?p=libvirt.git;a=commit;h=58eb4f2cbbb17b1afa1a9e0766a87f25aa895b73
     *  https://libvirt.org/git/?p=libvirt.git;a=commit;h=97e4f21782b49f6147a5fc5092740948fc89ede9
     */
    if (priv->nresources == 0 &&
        priv->hasRWDisks &&
        driver->requireLeaseForDisks) {
        virReportError(VIR_ERR_CONFIG_UNSUPPORTED, "%s",
                       _("Read/write, exclusive access, disks were present, but no leases specified"));
        return -1;
    }

    if (fd)
        *fd = -1;
    /* 
     * fd not used now
     *  https://libvirt.org/git/?p=libvirt.git;a=commit;h=5247b0695a1914e16d1b6333aff6038c0bd578dc
     *  https://libvirt.org/git/?p=libvirt.git;a=commit;h=64bdec384101f7a5e6989ee871b360c110ade571
     */

    if(!lockspace) {
        VIR_DEBUG("Process not open dlm lockspace, skipping release");
        goto cleanup;
    }

    /* readonly not acquire lock */
    if (!(flags & VIR_LOCK_MANAGER_ACQUIRE_REGISTER_ONLY)) {
        VIR_DEBUG("Acquiring object %zu", priv->nresources);

        ifd = open(driver->dlmFilePath, O_RDWR);
        if (ifd < 0) {
            virReportSystemError(errno, _("fail open %s"), driver->dlmFilePath);
            goto cleanup;
        }

        for (i = 0; i < priv->nresources; i++) {
            VIR_DEBUG("Acquiring object %zu", priv->nresources);

            memset(&lksb, 0, sizeof(lksb));
            rv = dlm_ls_lock_wait(lockspace, priv->resources[i].mode,
                                  &lksb, LKF_NOQUEUE|LKF_PERSISTENT,
                                  priv->resources[i].name, strlen(priv->resources[i].name),
                                  0, NULL, NULL, NULL);
            if (rv || lksb.sb_status) {
                // TODO: more human wrong report -- status==11(EAGAIN) means 'LKF_NOQUEUE was requested and the lock could not be granted'
                virReportSystemError(errno, 
                                     _("Failed to acquire lock, rv=%d lksb.sb_status=%d"),
                                     rv, lksb.sb_status);
                rv = -1;
                goto cleanup;
                // FIXME: when failed ???
            }

            theLock = virLockManagerDlmRecordLock(priv->resources[i].name, priv->resources[i].mode,
                                               lksb.sb_lkid, priv->vm_pid);
            if (!theLock) {
                // TODO, need unlock ?
                virReportError(VIR_ERR_INTERNAL_ERROR,
                               _("Fail record lock, resourceName=%s lockId = %d vm_pid=%jd"),
                               NULLSTR(priv->resources[i].name),
                               lksb.sb_lkid,
                               (intmax_t)priv->vm_pid);
                rv = -1;
                goto cleanup;
            }

            virLockManagerDlmWriteLock(theLock, ifd, 1);
        }

        if(VIR_CLOSE(ifd) < 0) {
            virReportSystemError(errno,
                                 _("Unable to save file %s"),
                                driver->dlmFilePath);
            goto cleanup;
        }
    }

    if (flags & VIR_LOCK_MANAGER_ACQUIRE_RESTRICT) {
        /* 
         * https://libvirt.org/git/?p=libvirt.git;a=commit;h=ebfb8c42434dd4d9f4852db2fde612351da500f7
         */
        dlm_close_lockspace(lockspace); // always return 0, so ignore return value
        lockspace = NULL;
    }

    rv = 0;

 cleanup:
    if (rv)
        VIR_FORCE_CLOSE(ifd);
    return rv;
}

static void virLockManagerDlmDeleteLock(const virLockInformationPtr lock, const char *dlmFilePath)
{
    int fd = -1;

    virMutexLock(&mutex);
    list_del(&lock->entry);
    virMutexUnlock(&mutex);

    fd = open(dlmFilePath, O_RDWR);
    if (fd < 0) {
        virReportSystemError(errno,
                             _("fail to open %s"), dlmFilePath);
        goto cleanup;
    }
    virLockManagerDlmWriteLock(lock, fd, 0); 
    if (VIR_CLOSE(fd) < 0) {
        virReportSystemError(errno,
                             _("Unable to save file %s"),
                             dlmFilePath);
        VIR_FORCE_CLOSE(fd);
    }

 cleanup:
    VIR_FREE(lock->name);
    VIR_FREE(lock);
}

static int virLockManagerDlmRelease(virLockManagerPtr lock,
                                    char **state,
                                    unsigned int flags)
{
    virLockManagerDlmPrivatePtr priv = lock->privateData;
    virLockManagerDlmResourcePtr resource = NULL;
    virLockInformationPtr theLock = NULL;
    struct dlm_lksb lksb = {0};
    int rv = -1;
    size_t i;

    virCheckFlags(0, -1);

    if(state)
        *state = NULL;

    if(!lockspace) {
        VIR_DEBUG("Process not open lockspace, skipping release");
        goto cleanup;
    }

    virReportError(VIR_ERR_INTERNAL_ERROR, _("priv->nresources=%zu lockListEmpty=%d"), priv->nresources, list_empty(&lockList));
    for (i = 0; i < priv->nresources; i++) {
        /* https://libvirt.org/git/?p=libvirt.git;a=commit;h=8dc93ffadcca0cc9813ba04036b7139922c55400
         */
        resource = priv->resources + i;

        list_for_each_entry (theLock, &lockList, entry) {
            if((theLock->vm_pid == priv->vm_pid) &&
               STREQ(theLock->name, resource->name) &&
               (theLock->mode == resource->mode)) {
                /* lock is held by another process, so whether `theLock->vm_pid == priv->vm_pid` or not is nothing */
                virReportError(VIR_ERR_INTERNAL_ERROR, _("find Lock, lockName=%s lockId=%d, vm_pid=%jd"), theLock->name, theLock->lkid, (intmax_t)theLock->vm_pid);
                rv = dlm_ls_unlock_wait(lockspace, theLock->lkid, 0, &lksb);
                if (!rv) {
                    virLockManagerDlmDeleteLock(theLock, driver->dlmFilePath);
                    break;
                }
                else { // TODO: when lock doesn't exist
                    virReportSystemError(errno,
                                         _("dlm unlock failed, lockName=%s, lockId=%d"),
                                         theLock->name, theLock->lkid);
                    goto cleanup;
                }
                break;
            }
        }
    }

    rv = 0;

 cleanup:
    return rv;
}

static int virLockManagerDlmInquire(virLockManagerPtr lock ATTRIBUTE_UNUSED,
                                    char **state,
                                    unsigned int flags)
{
    /* not support mannual lock, so this function almost does nothing */
    virCheckFlags(0, -1);

    if (state)
        *state = NULL;

    return 0;
}

virLockDriver virLockDriverImpl =
{
    .version = VIR_LOCK_MANAGER_VERSION,

    .flags = VIR_LOCK_MANAGER_USES_STATE, // currently not used

    .drvInit = virLockManagerDlmInit,
    .drvDeinit = virLockManagerDlmDeinit,

    .drvNew = virLockManagerDlmNew,
    .drvFree = virLockManagerDlmFree,

    .drvAddResource = virLockManagerDlmAddResource,

    .drvAcquire = virLockManagerDlmAcquire,
    .drvRelease = virLockManagerDlmRelease,
    .drvInquire = virLockManagerDlmInquire,
};
