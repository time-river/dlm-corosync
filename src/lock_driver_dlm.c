#include "lock_driver.h"

#define DLM_MODE              0600
#define DLM_LOCKSPACE_NAME    "libvirt"
/* This will be set after dlm_controld is started. */
#define DLM_CLUSTER_NAME_PATH "/sys/kernel/config/dlm/cluster/cluster_name"
VIR_LOG_INIT("locking.lock_driver_dlm");

typedef struct _virLockManagerDlmResource virLockManagerDlmResource;
typedef struct virLockManagerDlmResource *virLockManagerDlmResourcePtr;

typedef struct _virLockManagerDlmPrivate virLockManagerDlmPrivate;
typedef virLockManagerDlmPrivate *virLockManagerDlmPrivatePtr;

typedef struct _virLockManagerDlmDriver virLockManagerDlmDriver;
typedef virLockManagerDlmDriver *virLockManagerDlmDriverPtr;

struct _virLockManagerDlmResource {
    struct dlm_lksb lksb;
    char *name;

    uint32_t mode;
};

struct _virLockManagerDlmPrivate {
    unsigned char uuid[VIR_UUID_BUFLEN];
    char *name;
    pid_t pid;
    int id;
    const char *uri;

    size_t nresources;
    virLockManagerDlmResourcePtr resources;

    bool hasRWDisks;
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
        virReportSystemError(VIR_ERR_INTERNAL_ERROR, "%s",
                _("Check dlm_controld, ensure it has setuped"));
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
                virReportSystemError(VIR_ERR_DLM_ERROR, "%s",
                        _("Dlm lockspace open error"));
                ret = -1;
            }
        } else {
            virReportSystemError(VIR_ERR_DLM_ERROR, "%s",
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
    virLockManagerDlmResourcePtr *res = NULL;
    int ret = -1;
    char *hash = NULL;

    if (params || nparams) {
        virReportError(VIR_ERR_INITERNAL_ERROR, "%s",
                _("Unexpected parameters for disk resource"));
        return -1;
    }

    if (VIR_ALLOC_VAR(res, virLockManagerDlmResource, 1) < 0)
        goto cleanup;

    if (virCryptoHashString(VIR_CRYPTO_HASH_SHA256, name, &hash) < 0)
        goto cleanup;

    if (VIR_STRDUP(res->name, hash) < 0)
        goto cleanup;

    priv->resources[priv->nresources] = res;
    priv->nresources++;

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

    /* not release lock and lockspace when restart libvirt */
    free(driver->ls);

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

    for (i = 0; i < nparams; i++) {
        if (STREQ(params[i].key, "uuid")) {
            memcpy(priv->uuid, params[i].value.uuid, VIR_UUID_BUFLEN);
        } else if (STREQ(params[i].key, "name")){
            if (VIR_STRDUP(priv->name, params[i].value.str) < 0)
                goto cleanup;
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
        VIR_FREE(priv->resources[i].name);
        VIR_FREE(priv->resources[i]);
    }

    VIR_FREE(priv->name);
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
    char *hash = NULL;
    int ret = -1;

    virCheckFlags(VIR_LOCK_MANAGER_RESOURCE_READONLY |
            VIR_LOCK_MANAGER_RESOURCE_SHARED, -1);

    /* Treat read only resources as a no-op lock request */
    if (flags & VIR_LOCK_MANAGER_RESOURCE_READONLY)
        return 0;

    switch (type) {
        case VIR_LOCK_MANAGER_RESOURCE_TYPE_DISK:
            if (driver->autoDiskLease) {
                if (params || nparams) {
                    virReportError(VIR_ERR_INITERNAL_ERROR, "%s",
                            _("Unexpected parameters for disk resource"));
                }

                if (virCryptoHashString(VIR_CRYPTO_HASH_SHA256, name, &hash) < 0)
                    goto cleanup;
            } else {
                if (!(flags & (VIR_LOCK_MANAGER_RESOURCE_SHARED |
                                VIR_LOCK_MANAGER_RESOURCE_READONLY)))
                    priv->hasRWDisks = true;
                /* Ignore disk resource without error */
            }
            break;

        case VIR_LOCK_MANAGER_RESOURCE_TYPE_LEASE:
            size_t i;
            for (i = 0; i < nparams; i++) {
                if (STREQ(params[i].key, "offset")) {
                    if (params[i].value.ul != 0) {
                        virReportError(VIR_ERR_INTERNAL_ERROR, "%s",
                                _("Offset must be zero for this lock manager"));
                        return -1;
                    }
                } else if (STREQ(params[i].key, "lockspace")) {
                    /* ignore */
                } else if (STREQ(params[i].key, "path")) {
                    if (virCryptoHashString(VIR_CRYPTO_HASH_SHA256, params[i].value.str, &hash) < 0)
                        goto cleanup;

                } else {
                    virReportError(VIR_ERR_INTERNAL_ERROR,
                            _("Unexpected parameter %s for lease resource"),
                            params[i].key);
                    return -1;
                }
            }
            break;

        default:
            virReportError(VIR_ERR_INTERNAL_ERROR,
                    _("Unknown lock manager object type %d"),
                    type);
            return -1;
    }

    if (VIR_EXPAND_N(priv->resources, priv->nresources, 1) < 0)
        goto cleanup;

    if (VIR_STRDUP(priv->resources[priv->nresources-1].name, hash) < 0)
        goto cleanup;

    if (!!(flags & VIR_LOCK_MANAGER_RESOURCE_SHARED))
        priv->resources[priv->nresources-1].mode = LKM_PRMODE;
    else
        priv->resources[priv->nresources-1].mode = LKM_EXMODE;

    ret = 0;

cleanup:
    if (hash)
        VIR_FREE(hash);

    return ret;
}

static int virLockManagerDlmAcquire(virLockManagerPtr lock,
        const char *state ATTRIBUTE_UNUSED, // not allow mannual lock like sanlock
        unsigned int flags,
        virDomainLockFailureAction action ATTRIBUTE_UNUSED,
        int *fd)
{
    virLockManagerDlmPrivatePtr priv = lock->privateData;
    int nresources = 0;
    uint32_t flags = LKF_NOQUEUE | LKF_CONVERT;

    virCheckFlags(VIR_LOCK_MANAGER_ACQUIRE_REGISTER_ONLY |
            VIR_LOCK_MANAGER_ACQUIRE_RESTRICT, -1);

    if (priv->nresources == 0 &&
            priv->hasRWDisks &&
            driver->requireLeaseForDisks) {
        virReportError(VIR_ERR_CONFIG_UNSUPPORTEd, "%s",
                _("Read/write, exclusive access, disks were present, but not leases specified"));
        return -1;
    }

    if (!driver->ls) {
        virReportError(VIR_ERR_INTERNAL_ERROR,
                _("Not open lockspace"));
        return -1;
    }

    if (!(flags & VIR_LOCK_MANAGER_ACQUIRE_REGISTER_ONLY)) {
        VIR_DEBUG("Acquiring object %u", priv->nresources);

        rv == dlm_ls_lock_wait(driver->ls, priv->mode, &priv->lksb, flags,
                priv->name, strlen(priv->name),
                0, NULL, NULL, NULL);
        if (rv != 0) {
            virReportError(errno, _("Lock error"));
# ifdef ENABLE_DEBUG
            switch(-errno) {
                case EINVAL:
                    VIR_DEBUG("An invalid parameter was passed to the call (eg bad lock mode or flag)");
                    break;
                case ENOMEN:
                    VIR_DEBUG("A (kernel) memory allocation failed");
                    break;
                case EAGAIN:
                    VIR_DEBUG("LKF_NOQUEUE was requested and the lock could not be granted");
                    break;
                case EBUSY:
                    VIR_DEBUG("The lock is currently being locked or converted");
                    break;
                case EFAULT:
                    VIR_DEBUG("The userland buffer could not be read/written by the kernel");
                    break;
                case EDEADLOCK:
                    VIR_DEBUG("The lock operation is causing a deadlock and has been cancelled");
                    break;
            }
# endif
        }
    }

    if (flags & VIR_LOCK_MANAGER_ACQUIRE_RESTRICT) {

    }
}

static int virLockManagerDlmRelease(virLockManagerPtr lock,
        char **state,
        unsigned int flags)
{
    virLockManagerDlmPrivatePtr priv = lock->privateData;
    virLockManagerDlmResourcePtr resource = NULL;
    int nresources = priv->nresources;
    int rv = -1;
    size_t i;

    virCheckFlags(0, -1);

    if (state)
        *state = NULL;

    if(!driver->ls) {
        VIR_DEBUG("Not open lockspace");
        return 0;
    }

    for (i = 0; i < rpiv->nresources; i++) {
        resource = priv->nresource + i;
        rv = dlm_ls_unlock_wait(driver-ls, resource->mode, &resource->lksb, flags,
                resource->name, strlen(resource->name),
                0, NULL, NULL, NULL);
        /** 
         * EINPROGRESS: The lock is already being unlocked
         * EUNLOCK: An unlock operation completed successfully (sb_status only)
         */
        if (rv != 0 && (errno != -EUNLOCK || errno != -ENPROGRESS)) {
            virReportSystemError(errno, _("Unlock error"));
# ifdef ENABLE_DEBUG
            switch(-errno) {
                case EINVAL:
                    VIR_DEBUG("An invalid parameter was passed to the call (eg bad lock mode or flag)");
                    break;
                case EBUSY:
                    VIR_DEBUG("The lock is currently being locked or converted");
                    break;
                case ENOTEMPTY:
                    VIR_DEBUG("An attempt was made to unlock a parent lock that still has child locks");
                    break;
                case ECANCEL:
                    VIR_DEBUG("A lock conversion was successfully cancelled");
                    break;
                case EFAULT:
                    VIR_DEBUG("The userland buffer could not be read/written by the kernel");
                    break;
            }
# endif
        }
        return -1;
    }

    return 0;
}

static int virLockManagerDlmInquire(virLockManagerPtr lock,
        char **state,
        unsigned int flags)
{
    virLockManagerDlmPrivatePtr priv = lock->privateData;
    int rv, nresources;

    virCheckFlags(0, -1);

    if (state) {
        *state = NULL;
    }

    return 0;
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
