#include "lock_driver.h"
#include "list.h"

#define DLM_MODE    0600
#define DLM_LOCKSPACE_NAME  "libvirt"

/* This will be set after dlm_controld is started. */
#define DLM_CLUSTER_NAME_PATH "/sys/kernel/config/dlm/cluster/cluster_name"

VIR_LOG_INIT("locking.lock_driver_dlm");

typedef struct _virLockManagerDlmResource virLockManagerDlmResource;
typedef virLockManagerDlmResource *virLockManagerDlmResourcePtr;

typedef struct _virLockManagerDlmPrivate virLockManagerDlmPrivate;
typedef virLockManagerDlmPrivate *virLockManagerDlmPrivatePtr;

typedef struct _virLockManagerDlmDriver virLockManagerDlmDriver;
typedef virLockManagerDlmDriver *vir LockManagerDlmDriverPtr;

struct _virLockManagerDlmResource {
    struct list_head entry;
    char *name;
    uint32_t mode;
    uint32_t lkid;
    pid_t vm_pid;
};

struct _virLockManagerDlmPrivate {
    unsigned char uuid[VIR_UUID_BUFLEN];
    char *vm_name;
    pid_t pid;
    int vm_id;
    const char *uri;

    size_t nresources;
    virLockManagerDlmResourcePtr resources;

    bool hasRWDisks;
};

static virLockManagerDlmDriverPtr driver = NULL;
static dlm_lshandle_t lockspace = NULL;
static struct list_head lockList;

static int virLockManagerDlmDeinit(void);

static int virLockManagerDlmInit(unsigned int version,
                                 const char *configFile,
                                 unsigned int flags)
{
    VIR_DEBUG("version=%u configFile=%s flags=0x%x", version, NULLSTR(configFile), flags);

    virCheckFlags(0, -1);

    if (driver)
        return 0;

    if (VIR_ALLOC(driver) < 0)
        return -1;

    driver->autoDiskLeases = true;
    driver->mode = DLM_MODE;
    if (VIR_STRDUP(driver->dlmClusterNamePath, DLM_CLUSTER_NAME_PATH) < 0) {
        VIR_FREE(driver);
    }
    if (VIR_STRDUP(driver->lockspaceName, DLM_LOCKSPACE_NAME) < 0) {
        VIR_FREE(driver->dlmDlusterNamePath);
        VIR_FREE(driver);
        goto cleanup;
    }

    if (virLockManagerDlmLoadConfig(configFile) < 0)
        goto cleanup;

    if (driver->autoDiskLease) {
        if (virLockManagerDlmSetupLockspace(driver) < 0)
            goto cleanup;
    }

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
    VIR_FREE(driver);

    return 0;
}

static int virLockManagerDlmNew(virLockManagerPtr lock,
                                unsigned int type,
                                size_t nparams,
                                virLockManagerPrarmPtr params,
                                unsigned int flags)
{
    virLockManagerDlmPrivatePtr priv;
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
        
        } else if (STREQ(params[i].key, "name")) {
        
        } else if (STREQ(params[i].key, "id")) {
        
        } else if (STREQ(params[i].key, "pid")) {
        
        } else if (STREQ(params[i].key, "uri")) {
        
        } else {
        
        }
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
        break;

    case VIR_LOCK_MANAGER_RESOURCE_TYPE_LEASE:
        break;

    default:
        virReportError(VIR_ERR_INTERNAL_ERROR,
                _("Unknown lock manager object type %d"),
                type);
        return -1;
    }

    // TODO
 cleanup:
    if (newName)
        VIR_FREE(newName);

    return ret;
}

static int virLockManagerDlmAcquire(virLockManagerPtr lock,
                                    const char *state ATTRIBUTE_UNUSED,
                                    unsigned int flags,
                                    virDomainLockFailureAction action ATTRIBUTE_UNUSED,
                                    int *fd, ATTRIBUTE_UNUSED)
{
    virLockManagerDlmPrivatePtr priv = lock->privateData;
    int nresources = 0;
    
    virCheckFlags(VIR_LOCK_MANAGER_ACQUIRE_REGISTER_ONLY |
                  VIR_LOCK_MANAGER_ACQUIRE_RESTRICT, -1);

    if (priv->nresources == 0 &&
        priv->hasRWDisks &&
        driver->requireLeaseForDisks) {
        virReportError(VIR_ERR_CONFIG_UNSUPPORTED, "%s",
                       _("Read/write, exclusive access, disks were present, but no leases specified"));
        return -1;
    }

    if (!ls) {
        // TODO
    }
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
