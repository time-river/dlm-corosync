#include "lock_driver.h"

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

    // TODO: other settings

    driver->requireLeaseForDisks = !river->autoDiskLeases;
    if (virConfGetValueString(conf, "require_lease_for_disks", &driver->requireLeaseForDisks) < 0)
        goto cleanup;

    ret = 0;

 cleanup:
    virConfFree(conf);
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

    // TODO

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

    if (driver)
        return 0;

    if (VIR_ALLOC(driver) < 0)
        return -1;

    driver->autoDiskLeases = true;

    if (virLockManagerDlmLoadConfig(configFile) < 0)
        goto error;

    if (driver->autoDiskLease) {
        // TODO: when auto disk lease config...
    }

    return 0;

 error:
    virLockManagerDlmDeinit();
    return -1;
}

static int virLockManagerDlmDeinit(void)
{
    if (!driver)
        return 0;

    // TODO release memory
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

 error:
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
