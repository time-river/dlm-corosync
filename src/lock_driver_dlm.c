#include "lock_driver.h"

#define DLM_LOCKSPACE_NAME    "libvirt"
/* This will be set after dlm_controld is started. */
#define DLM_CLUSTER_NAME_PATH "/sys/kernel/config/dlm/cluster/cluster_name"

VIR_LOG_INIT("locking.lock_driver_dlm");

typedef struct _virLockManagerDlmPrivate virLockManagerDlmPrivate;
typedef virLockManagerDlmPrivate *virLockManagerDlmPrivatePtr;

typedef struct _virLockManagerDlmDriver virLockManagerDlmDriver;
typedef virLockManagerDlmDriver *virLockManagerDlmDriverPtr;

struct _virLockManagerDlmPrivate {
    unsigned char uuid[VIR_UUID_BUFLEN];
    char *name;
    pid_t pid;
    int id;
    const char *uri;

    bool hasRWDisks;
    size_t nresources;
    // TODO: dlm-corosync vars 

    unsigned int flags;

    /* whether the VM was registered or not */
    bool registered;
};

struct _virLockManagerDlmDriver {
    bool autoDiskLease;
    bool requireLeaseForDisks;
    char *lockspaceName;
    dlm_lshandle_t ls;
}

static virLockManagerDlmDriverPtr driver;

static int virLockManagerDlmLoadConfig(const char *configFile)
{
    virConfPtr conf;
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

    if (virConfGetValueBool(conf, "auto_disk_leases", &driver->autoDiskLease) < 0)
        goto cleanup;

    if (virConfGetValueString(conf, "lockspace_name", &driver->lockspaceName) < 0)
        goto cleanup;

    driver->requireLeaseForDisks = !driver->autoDiskLease;
    if (virConfGetValueString(conf, "require_lease_for_disks", &driver->requireLeaseForDisks) < 0)
        goto cleanup;

    ret = 0;

 cleanup:
    virConfFree(conf);
    return ret;
}

static int virLockManagerDlmSetupLockspace(virLockManagerDlmDriverPtr driver)
{
    int ret = -1;
    dlm_lshandle_t ls;

    /* check dlm_controld */
    ret = access(DLM_CLUSTER_NAME_PATH, O_REONLY);
    if (ret < 0) {
        // TODO: change error type
        virReportSystemError(VIR_ERR_INTERNAL_ERROR, "%s",
                             _("Check dlm_controld"));
    } else {
        ls = dlm_create_lockspace(driver->lockspaceName, MODE);
        if (!ls) {
            driver->ls = ls;
            ret = 0;
        } else if (!ls && errno == -EEXIST) {
            ls = dlm_open_lockspace(driver->lockspaceName);
            if(!ls) {
                driver->ls = ls;
                ret = 0;
            } else {
                // TODO: change error type
                virReportSystemError(VIR_ERR_INTERNAL_ERROR, "%s",
                                     _("Dlm lockspace open error"));
                ret = -1;
            }
        } else {
            // TODO: change error type
            virReportSystemError(VIR_ERR_INTERNAL_ERROR, "%s",
                                 _("Dlm locksapce create error"));
            ret = -1;
        }
    }

    return ret;
}

static int virLockManagerDlmAddDisk(virLockManagerDlmDriverPtr driver,
                                    virLockManagerPtr lock,
                                    const char *name,
                                    size_t nparams,
                                    virLockManagerParamPtr params ATTRIBUTE_UNUSED,
                                    bool shared)
{
    virLockManagerDlmPrivatePtr priv = lock->privateData;
    int ret = -1;
    // TODO: resource
    
    char *hash = NULL;

    if (params || nparams) {
        virReportError(VIR_ERR_INITERNAL_ERROR, "%s",
                       _("Unexpected parameters for disk resource"));
        return -1;
    }

    // TODO: complete
    
    if (virCryptoHashString(VIR_CRYPTO_HASH_SHA256, name, &hash) < 0)
        goto cleanup;

    // TODO: fill component to `"struct dlm"`

    ret = 0;

 cleanup:
    if (ret == -1)
        VIR_FREE(res);
    VIR_FREE(hash);
    return ret;
}

static int virLockManagerDlmAddLease(virLockManagerPtr lock,
                                     const char *name,
                                     size_t nparams,
                                     virLockManagerParamPtr params,
                                     bool shared)
{
    virLockManagerDlmPrivatePtr priv = lock->privateData;
    int ret = -1;
    // TODO: define resource
    size_t i;

    // TODO
    
    return ret;
}

static int virLockManagerDlmDeinit(void);

static int virLockManagerDlmInit(unsigned int version,
                                 const char *configFile,
                                 unsigned int flags)
{
    VIR_DEBUG("version=%u configFile=%s flags=0x%x", version, NULLSTR(configFile), flags);

    virCheckFlags(0, -1);

    /* create lockspace needs previleges */
    if (geteuid() != 0) {
        virReportError(VIR_ERR_INTERNAL_ERROR,
                       _("Dlm lock requires superuser privileges"));
        return -1;
    }

    if (driver)
        return 0;

    if (VIR_ALLOC(driver) < 0)
        return -1;

    driver->autoDiskLeases = true;
    if (VIR_STRDUP(driver->lockspaceName, DLM_LOCKSPACE_NAME) < 0) {
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
    virLockManagerDlmDeinit();
    return -1;
}

static int virLockManagerDlmDeinit(void)
{
    if (!driver)
        return 0;

    /* force release lockspace if exist */
    if (driver->ls) {
        dlm_release_lockspace(driver->lockspaceName, driver->ls, 1);

    VIR_FREE(driver->lockspaceName);
    VIR_FREE(driver);

    return 0;
}

static int virLockManagerDlmNew(virLockManagerPtr lock,
                                unsigned int type,
                                size_t nparams,
                                virLockManagerParamPtr params,
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

    // mark whether new started or not
    priv->flags = flags;

    for (i = 0; i < nparams; i++) {
        if (STREQ(params[i].key, "uuid")) {
            memcpy(priv->uuid, params[i].value.uuid, VIR_UUID_BUFLEN);
        } else if (STREQ(params[i].key, "name")){
            if (VIR_STRDUP(priv->name, params[i].value.str) < 0)
                return -1;
        } else if (STREQ(params[i].key, "id")) {
            priv->id = params[i].value.iv;
        } else if (STREQ(params[i], "pid")) {
            priv->pid = params[i].value.iv;
        } else if (STREQ(params[i].key, "uri")){
            priv->vm_uri = params[i].value.cstr;
        } else {
            virReportError(VIR_ERR_INTERNAL_ERROR,
                           _("Unexpected parameter %s for object"),
                           params[i].key);
        }
    }

    // TODO: check args or provide args to remote to check
    if (!(flags & VIR_LOCK_MANAGER_NEW_STARTED) &&
            1)
        priv->registered = true;

    lock->privateData = priv;
    return 0;

 cleanup:
    VIR_FREE(priv);
    return -1;
}

struct void virLockManagerDlmFree(virLockManagerPtr lock)
{
    virLockManagerDlmPrivatePtr priv = lock->privateData;
    size_t i;

    if(!priv)
        return;

    for (i = 0; i < priv->nresources; i++) {
        // TODO: release resources args
    }

    VIR_FREE(priv->resources.name);
    VIR_FREE(priv);
    lock->privateData = NULL;

    return;
}

static int virLockManagerDlmAddResource(virLockManagerPtr lock,
                                        unsigned int type,
                                        const char *name,
                                        size_t nparams,
                                        virLockManagerParamPtr params,
                                        unsigned int flags)
{
    virLockManagerDlmPrivatePtr priv = lock->privateData;
    
    virCheckFlags(VIR_LOCK_MANAGER_RESOURCE_READONLY |
                  VIR_LOCK_MANAGER_RESOURCE_SHARED, -1);

    // TODO: maybe need check the max

    /* Treat read only resources as a no-op lock request */
    if (flags & VIR_LOCK_MANAGER_RESOURCE_READONLY)
        return 0;

    switch (type) {
        case VIR_LOCK_MANAGER_RESOURCE_TYPE_DISK:
            if (driver->autoDiskLease) {
            // TODO
                if (virLockManagerDlmAddDisk(driver, lock, name, nparams, params,
                                             !!(flags & VIR_LOCK_MANAGER_RESOURCE_SHARED)) < 0)
                    return -1;
                
                if (virLockManagerDlmCreateLease(driver, ...) < 0)
                    return -1;

            } else {
                if (!(flags & (VIR_LOCK_MANAGER_RESOURCE_SHARED |
                               VIR_LOCK_MANAGER_RESOURCE_READONLY)))
                    priv->hasRWDisks = true; // TODO for hasRWDisks
                /* Ignore disk resource without error */
            }
            break;

        case VIR_LOCK_MANAGER_RESOURCE_TYPE_LEASE:
            // TODO
            if (virLockManagerDlmAddLease(lock, name, nparams, params,
                                          !!(flags & VIR_LOCK_MANAGER_RESOURCE_SHARED)) < 0)
                return -1;
            break;

        default:
            virReportError(VIR_ERR_INTERNAL_ERROR,
                           _("Unknown lock manager object type %d"),
                           type);
            return -1;
    }

    return 0;
}

static int virLockManagerDlmAcquire(virLockManagerPtr lock,
                                    const char *state,
                                    unsigned int flags,
                                    virDomainLockFailureAction action ATTRIBUTE_UNUSED,
                                    int *fd)
{
    virLockManagerDlmPrivatePtr priv = lock->privateData;
    int nresources = 0;

    virCheckFlags(VIR_LOCK_MANAGER_ACQUIRE_REGISTER_ONLY |
                  VIR_LOCK_MANAGER_ACQUIRE_RESTRICT, -1);

    if (priv->nresources == 0 &&
        priv->hasRWDisks &&
        driver->requireLeaseForDisks) {
        virReportError(VIR_ERR_CONFIG_UNSUPPORTEd, "%s",
                       _("Read/write, exclusive access, disks were present, but not leases specified"));
        return -1;
    }

    /**
     * only initialize if in the read child process
     */
    if (priv->pid == getpid()) {
        // TODO: acquire lock
        // register
        priv->registered = true;
    } else if (!priv->registered) {
        VIR_DEBUG("Process not registered, not acquiring lock");
        return 0;
    }

    // TODO
    if (!(flags & VIR_LOCK_MANAGER_ACQUIRE_REGISTER_ONLY)) {
        
    }

    if (flags & VIR_LOCK_MANAGER_ACQUIRE_RESTRICT) {
    
    }
}

static int virLockManagerDlmRelease(virLockManagerPtr lock,
                                    char **state,
                                    unsigned int flags)
{
    virLockManagerDlmPrivatePtr priv = lock->privateData;
    int nresources = priv->nresources;
    int rv;

    virCheckFlags(0, -1);

    if(!priv->registered) {
        VIR_DEBUG("Process not registered, skipping release");
        return 0;
    }

    if (state) {
        // TODO
    }
}

static int virLockManagerDlmInquire(virLockManagerPtr lock,
                                    char **state,
                                    unsigned int flags)
{
    virLockManagerDlmPrivatePtr priv = lock->privateData;
    int rv, nresources;

    virCheckFlags(0, -1);

    // TODO
}

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
