#define _REENTRANT

#include "lock_driver.h"
#include "list.h"
#include <libdlm.h>

#define DLM_LOCKSPACE_MODE  0600
#define DLM_LOCKSPACE_NAME  "libvirt"

#define PRMODE  "PRMODE"
#define EXMODE  "EXMODE"

#define STATUS             "STATUS"
#define RESOURCE_NAME      "RESOURCE_NAME"
#define LOCK_ID            "LOCK_ID"
#define LOCK_MODE          "LOCK_MODE"
#define PID                "QEMU_PID"

/* This will be set after dlm_controld is started. */
#define DLM_CLUSTER_NAME_PATH "/sys/kernel/config/dlm/cluster/cluster_name"

VIR_LOG_INIT("locking.lock_driver_dlm");

typedef struct _virLockInformation virLockInformation;
typedef virLockInformation virLockInformationPtr;

typedef struct _virLockManagerDlmResource virLockManagerDlmResource;
typedef virLockManagerDlmResource *virLockManagerDlmResourcePtr;

typedef struct _virLockManagerDlmPrivate virLockManagerDlmPrivate;
typedef virLockManagerDlmPrivate *virLockManagerDlmPrivatePtr;

typedef struct _virLockManagerDlmDriver virLockManagerDlmDriver;
typedef virLockManagerDlmDriver *vir LockManagerDlmDriverPtr;

struct _virLockInformation {
    struct list_head entry;
    char *name;
    uint32_t mode;
    uint32_t lkid;
    pid_t vm_pid;
};

struct _virLockManagerDlmResource {
    char *name;
    uint32_t mode;
    pid_t vm_pid;
};

struct _virLockManagerDlmPrivate {
    unsigned char vm_uuid[VIR_UUID_BUFLEN];
    char *vm_name;
    pid_t vm_pid;
    int vm_id;
    const char *vm_uri;

    size_t nresources;
    virLockManagerDlmResourcePtr resources;

    bool hasRWDisks;
};

struct _virLockManagerDlmDriver {
	bool adoptLock;
	char *lockspaceName;
    char *dlmFilePath;
};

static virLockManagerDlmDriverPtr driver = NULL;
static dlm_lshandle_t lockspace = NULL;
static struct list_head lockList;

static int virLockManagerDlmLoadConfig(const char *configFile)
{
    virConfPtr conf = NULL;
    int ret = -1;

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

static int virLockManagerToModeUint(const char *token)
{
    if (!strcmp(mode, PRMODE))
        return LKM_PRMODE;
    if (!strcmp(mode, EXMODE))
        return LKM_EXMODE;

    return 0;
}

static char *virLockManagerToModeText(const uint32_t mode)
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

static void virLockManagerRecordLock(const char *name, const uint32_t mode
                                     const uint32_t lkid, const pid_t vm_pid)
{
    virLockInformationPtr lock = NULL;
    int fd;

    if (VIR_ALLOC(lock) < 0)
       return;

    if (VIR_STRDUP(lock->name, name) < 0)
       return; 

    lock->mode = mode;
    lock->lkid = lkid;
    lock->vm_pid = vm_pid;

    list_add_tail(&lock->entry, &lockList);

    return;
}

static void virLockManagerWriteLock(const virLockInformationPtr lock, const int fd,
                                    const bool status)
{
    char buffer[BUFSIZ] = {0};
    off_t offet = 0, ret = 0;

    /* <pid>\n
     *    10 1
     * STATUS RESOURCE_NAME LOCK_MODE      PID
     *      6            32         9       10        
     */
    offset = 11 + 61 * lock->lkid;
	ret = lseed(fd, offset, SEEK_SET);
	if (ret < 0) {
		virReportSystemError(errno, "%s",
							 _("lseek failed"));
        return;
    }
    
    snprintf(buffer, sizeof(buffer), "%6d %32s %9s %10d\n", \
             status, lock->name, virLockManagerToModeText(lock->mode), lock->vm_pid);
    if (safewrite(fd, buffer, strlen(buffer) != strlen(buffer))) {
        virReportSystemError(errno, "%s",
                             _("write lock failed"));
        return;
    }

    fdatasync(fd);

    return;
}

static void virLockManagerAdoptLock(char *raw) {
    char *str = NULL, *subtoken = NULL, *saveptr = NULL;
    int i = 0, status = 0;
    char *name = NULL;
    uint32_t mode;
    struct dlm_lksb lksb = {0};
    pid_t vm_pid;

    for (i = 0, str = raw, status = 0; ; str = NULL, i++) {
        subtoken = strtok_r(str, " \n", &saveptr);
        if (subtoken == NULL)
            break;

        switch(i) {
        case 0:
            status = atoi(subtoken);
            break;
        case 1:
            if (VIR_STRDUP(subtoken, name) != 1)
                status = 0;
            break;
        case 2:
            mode = virLockManagerToModeUint(subtoken);
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
        virReportSystemError(errno, "%s",
                             _("adopt lock failed"));
    }

    virLockManagerRecordLock(name, mode, lksb.sb_lkid, vm_pid);

    return;

 out:
    if (name)
        VIR_FREE(name);

    return;
}

static int virLockManagerPrepareLockfile(const char *dlmFilePath, const bool adoptLock)) {
    FILE *fp;
    int line = 0;
    ssize_t count = 0;
    size_t n = 0;
    pid_t previous = 0;
    char *buffer = NULL;
    uint32_t nodeid = 0;
    int ret = -1;

    if (!access(dlmFilePath, F_OK)) {
        fp = fopen(dlmFile, "r");
        if (!fp) {
            virReportSystemError(errno, "%s",
                                 _("open dlm lockfile failed"));
            goto out;
        }

        for (line = 0; !feof(fp); line++) {
            count = getline(&buffer, &n, fp);
            if (count <= 0)
                break;

            switch (line) {
            case 0:
                previous = atoi(buf);
                if (!previous)
                    goto out;
                break;
            case 1:
                break;
            default:
                virLockManagerAdoptLock(buffer);
                break;
            }

            if (!adoptLock)
                break;
        }

        VIR_FREE(buffer);

        if (adoptLock && !virLockManagerGetNodeid(&nodeid))
            dlm_ls_purge(lockspace, nodeid, previous);
        }

    }

    ret = 0;

 out:
    return ret;
}

static int virLockManagerDumpLockfile(const char *dlmFilePath)
{
    virLockInformationPtr lock = NULL;
    char buffer[BUFSIZ] = {0};
    int fd = -1, ret = -1;

    fd = open(dlmFilePath, O_WRONLY|O_CREAT|O_TRUNC);
    if (fd < 0) {
        virReportSystemError(errno, "%s",
                             _("open dlm file failed"));
        return -1;
    }
    
    snprintf(buffer, sizeof(buffer), "%10d\n%6s %32s %9s %10s\n" \
             getpid(), STATUS, RESOURCE_NAME, LOCK_MODE, PID);
    if (safewrite(fd, buffer, strlen(buffer)) != strlen(buffer)) {
        virReportSystemError(VIR_ERR_OPERATION_FAILED,
                             _("failed to write file, '%s'"), dlmFilePath);
        ret = -errno;
        goto out;
    }

    list_for_each_entry(lock, &lockList, entry) {
        virLockManagerWriteLock(lock, fd, 1);
    }

 out:
    VIR_CLOSE_FORCE(fd);
    return ret;
}

static int virLockManagerSetupLockfile(const char *dlmFilePath, const bool adoptLock)
{
    if (virLockManagerPrepareLockfile(dlmFilePath, adoptLock)) {
        virReportSystemError(VIR_ERR_INTERNAL_ERROR, "%s",
                             _("dlm prepare lockfile failed"));
        return -1;
    }

    if (virLockManagerDumpLockfile(dlmFilePath)) {
        virReportSystemError(VIR_ERR_INTERNAL_ERROR, "%s",
                             ("dlm dump lockfile failed"));
        return -1;
    }

    return 0;
}

static int virLockManagerDlmSetupDlm(virLockManagerDlmDriverPtr driver)
{
    int ret = -1;

    list_head_init(&lk_list);

    /* check dlm is running */
    if (access(DLM_CLUSTER_PATH, F_OK)) {
        virReportSystemError(errno, "%s",
                             _("Check dlm_controld, ensure it has setuped"));
    }

    /* open lockspace */
    lockspace = dlm_open_lockspace(DLM_LOCKSPACE_NAME);
    if (!lockspace)
        lockspace = dlm_create_lockspace(DLM_LOCKSPACE_NAME, DLM_LOCKSPACE_MODE);
    if (!lockspace) {
        virReportSystemError(errno, "%s",
                             _("dlm create lockspace failed"));
    }

    if (dlm_ls_pthread_init(lockspace)) {
        if (errno != EEXIST)
            virReportSystemError(errno, "%s",
                                 _("dlm thread initialize failed"));
    }

    if (virLockManagerSetupLockfile(driver->dlmFilePath, driver->adoptLock)) {
        virReportSystemError(VIR_ERR_INTERNAL_ERROR, "%s",
                             _("setup lockfile failed"));
    }

}

static int virLockManagerDlmDeinit(void);

static int virLockManagerDlmInit(unsigned int version,
                                 const char *configFile,
                                 unsigned int flags)
{
    VIR_DEBUG("version=%u configFile=%s flags=0x%x", version, NULLSTR(configFile), flags);

    virCheckFlags(0, -1);

    if (geteuid() != 0) {
        virReportError(VIR_ERR_INTERNAL_ERROR,
                       _("Dlm lock requires superuser privileges"));
        return -1;
    }

    if (driver)
        return 0;

    if (VIR_ALLOC(driver) < 0)
        return -1;

    driver->adoptLock = true;
    driver->mode = DLM_MODE;

    if (VIR_STRDUP(driver->lockspaceName, DLM_LOCKSPACE_NAME) < 0)
        goto cleanup;

    if (VIR_STRDUP(driver->dlmFilePath, DLM_FILE_PATH) < 0)
        goto cleanup;

    if (virLockManagerDlmLoadConfig(configFile) < 0)
        goto cleanup;

    if (virLockManagerDlmSetupDlm(driver) < 0)
        goto cleanup;

    return 0;

 cleanup:
    virLockManagerLockDaemonDeinit();
    return -1;
}

static int virLockManagerLockDaemonDeinit(void)
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
                                virLockManagerPrarmPtr params,
                                unsigned int flags)
{
    virLockManagerDlmPrivatePtr priv = NULL;
    size_t i;

    virCheckFlags(VIR_LOCK_MANAGER_NEW_STATED, -1);

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
                goto cleanup;
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

    if (priv->pid == 0)
        VIR_DEBUG("Missing PID parameter for domain object");
    if (!priv->name) {
        virReportError(VIR_ERR_INTERNAL_ERROR, "%s",
                       _("Missing name parameter for domain object"));
        return -1;
    }

    lock->privateData = priv;
    return 0;

 cleanup:
    VIR_FREE(priv);
    return -1;
}

static void virLockManagerDlmFree(virLockManagerPtr lock)
{
    virLockManagerDlmPrivatePtr priv = lock->privateData;
    size_t i;

    if (!priv)
        return;

    for (i = 0; i < priv->nresources; i++) {
   		VIR_FREE(priv->resources[i].vm_name);
// TODO 
    }

    VIR_FREE(priv);
    lock->privateData = NULL;
}

static int virLockManagerDlmAddResource(virLockManagerPtr lock,
                                        unsigned int type, const char *name,
                                        size_t nparams,
                                        virLockManagerParamPtr params,
                                        unsigned int flags)
{
    virLockManagerLockDaemonPrivatePtr priv = lock->privateData;
    char *newName = NULL;
    int ret = -1;

    virCheckFlags(VIR_LOCK_MANAGER_RESOURCE_READONLY |
            VIR_LOCK_MANAGER_RESOURCE_SHARED, -1);

    /* Treat read only resources as a no-op lock request */
    if (flags & VIR_LOCK_MANAGER_RESOURCE_READONLY)
        return 0;

    switch (type) {
    case VIR_LOCK_MANAGER_RESOURCE_TYPE_DISK:
        /* https://libvirt.org/git/?p=libvirt.git;a=commit;h=97e4f21782b49f6147a5fc5092740948fc89ede9
         */
            if (!(flags & (VIR_LOCK_MANAGER_RESOURCE_SHARED |
                           VIR_LOCK_MANAGER_RESOURCE_READONLY))) {
                priv->hasRWDisks = true;
                /* ignore disk resource without error */
                return 0;
            }

            if (params || narams) {
                virReportError(VIR_ERR_INITERNAL_ERROR, "%s",
                               _("Unexpected parameters for disk resource"));
            }

            if (virCryptoHashString(VIR_CRYPTO_HASH_SHA256, name, &newName) < 0)
                goto cleanup;
        break;

    case VIR_LOCK_MANAGER_RESOURCE_TYPE_LEASE:
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

    if (VIR_STRDUP(priv->resources[priv->nresources-1].name, newName) < 0)
        goto cleanup;

    if (!!(flags & VIR_LOCK_MANAGER_RESOURCE_SHARED))
        priv->resources[priv->nresources-1].mode = LKM_PRMODE;
    else
        priv->resources[priv->nresources-1].mode = LKM_EXMODE;

    ret = 0;

 cleanup:
    if (newName)
        VIR_FREE(newName);

    return ret;
}

static int virLockManagerDlmAcquire(virLockManagerPtr lock,
                                    const char *state ATTRIBUTE_UNUSED,
                                    unsigned int flags,
                                    virDomainLockFailureAction action ATTRIBUTE_UNUSED,
                                    int *fd)
{
    virLockManagerDlmPrivatePtr priv = lock->privateData;
    virLockInformationPtr lock = NULL;
    struct dlm_lskb lskb = {0};
    int nresources = 0;
    int rv = -1, fd = -1;
    size_t i;

    virCheckFlags(VIR_LOCK_MANAGER_ACQUIRE_REGISTER_ONLY |
                  VIR_LOCK_MANAGER_ACQUIRE_RESTRICT, -1);

    if (priv->nresources == 0 &&
        priv->hasRWDisks &&
        driver->requireLeaseForDisks) {
        virReportError(VIR_ERR_CONFIG_UNSUPPORTED, "%s",
                       _("Read/write, exclusive access, disks were present, but no leases specified"));
        return -1;
    }

    if (fd)
        fd = -1;
    /* 
     * fd not used now
     *  https://libvirt.org/git/?p=libvirt.git;a=commit;h=5247b0695a1914e16d1b6333aff6038c0bd578dc
     *  https://libvirt.org/git/?p=libvirt.git;a=commit;h=64bdec384101f7a5e6989ee871b360c110ade571
     */

    if(!lockspace) {
        VIR_DEBUG("Process not open lockspace, skipping release");
        goto cleanup;
    }

    if (!(flags & VIR_LOCK_MANAGER_ACQUIRE_REGISTER_ONLY)) {
        VIR_DEBUG("Acquiring object %u", priv->nresources);

        for (i = 0; i < priv->nresources; i++) {
            VIR_DEBUG("Acquiring object %u", priv->nresources);

            memset(&lskb, 0, sizeof(lskb));
            rv = dlm_ls_lock_wait(lockspace, priv->resources[i].mode,
                                  LKF_NOQUEUE|LKF_PERSISTENT,
                                  priv->resources[i].name, strlen(priv->resources[i].name),
                                  0, NULL, NULL, NULL);
            if (!rv || !lksb.sb_status) {
                virReportError(errno,
                               _("Failed to acquire lock"));
                goto error;
            }

            lock = virLockManagerRecordLock(priv->resources[i].name, priv->resources[i].mode,
                                            lksb.sb_lkid, priv->resources[i].vm_pid);
            if (!lock)
                // TODO
            fd = open(driver->dlmFilePath, O_RDWR);
            if (fd < 0) {
                virReportError(errno,
                               _("fail open %s", driver->dlmFilePath));
                goto error;
            }
            virLockManagerWriteLock(lock, fd, 1);
        }
    }

    if (flags & VIR_LOCK_MANAGER_ACQUIRE_RESTRICT) {
        /* 
         * https://libvirt.org/git/?p=libvirt.git;a=commit;h=ebfb8c42434dd4d9f4852db2fde612351da500f7
         */
        dlm_close_lockspace(lockspace);
        lockspace = NULL;
    }

    rv = 0;

 error:
    return rv;
}

static void virLockManagerDeleteLock(const virLockInformationPtr lock, const char *dlmFilePath)
{
    int fd = -1;

    list_del(&lock->entry);

    fd = open(dlmFilePath, O_RDWR);
    if (fd < 0) {
        virReportError(errno,
                       _("fail to open %s", dlmFilePath));
        goto cleanup;
    }
    virLockManagerWriteLock(lock, fd, 0); 

 cleanup:
    VIR_CLOSE(fd);
    VIR_FREE(lock->name);
    VIR_FREE(lock);
}

static int virLockManagerDlmRelease(virLockManagerPtr lock,
                                    char **state,
                                    unsigned int flags)
{
    virLockManagerDlmPrivatePtr priv = lock->privateData;
    virLockManagerDlmResourcePtr resource = NULL;
    virLockInformationPtr lock = NULL;
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

    for (i = 0; i < priv->nresources; i++) {
        /* https://libvirt.org/git/?p=libvirt.git;a=commit;h=8dc93ffadcca0cc9813ba04036b7139922c55400
         */
        resource = priv->resources + i;

        list_for_each_entry (lock, &lk_list, entry) {
            if(STREQ(lock->name, resource->name) && (lock->mode == lock->mode)) {
                rv = dlm_ls_unlock_wait(lockspace, lock->lkid, 0, &lksb);
                if (!rv) {
                    virLockManagerDeleteLock(lock, driver->dlmFilePath);
                    break;
                }
                else {
                    virReportError(errno,
                                   _("dlm unlock failed, lock id: %d", lock->lkid));
                    goto cleanup;
                }
            }
        }
    }

    rv = 0;

 cleanup:
    return rv;
}

static int virLockManagerDlmInquire(virLockManagerPtr lock ATTRIBUTED_UNSED,
                                    char **state,
                                    unsigned int flags)
{
    /* not support mannual lock, so this function almost does nothing */
    virCheckFlags(0, -1);

    if (state)
        *state = NULL;

    return 0;
}

struct _virLockManagerDlmDriver
virLockDriver virLockDriverImpl =
{
    .version = VIR_LOCK_MANAGER_VERSION,

    .flags = VIR_LOCK_MANAGER_USES_STATE,

    .drvInit = virLockManagerDlmInit,
    .drvDeinit = virLockManagerDlmDeinit,

    .drvNew = virLockManagerDlmNew,
    .drvFree = virLockManagerDlmFree,

    .drvAddResource = virLockManagerDlmAddResource,

    .drvAcquire = virLockManagerDlmAcquire,
    .drvRelease = virLockManagerDlmRelease,
    .drvInquire = virLockManagerDlmInquire,
};
