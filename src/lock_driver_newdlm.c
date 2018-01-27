#include <stdint.h>
#include "lock_driver.h"

#define DLM_MODE            0600
#define DLM_LOCKSPACE_NAME  "libvirt"

VIR_LOG_INIT("locking.lock_driver_dlm");

typedef struct _virLockManagerDlmResource virLockManagerDlmResource;
typedef virLockManagerDlmResource *virLockManagerDlmResourcePtr;

typedef struct _virLockManagerDlmPrivate  virLockManagerDlmPrivate;
typedef virLockManagerDlmPrivate *virLockManagerDlmPrivatePtr;

typedef struct _virLockManagerDlmDriver virLockManagerDlmDriver;
typedef virLockManagerDlmDriver *virLockManagerDlmDriverPtr;

struct _virLockManagerDlmResource {
    char *lockspace;
    char *name;     // resource name
    uint32_t mode; // lock flags
}

struct _virLockManagerDlmPrivate {
    unsigned char uuid[VIR_UUID_BUFLEN];
    char *vm_name;
    int id;
    pid_t pid;

    size_t nresources;
    virLockManagerDlmResourcePtr resources;

    bool hasRWDisks;
}

struct _virLockManagerDlmDriver {
    bool autoDiskLease;
    bool requireLeaseForDisks;

    char *lockspaceName;
    /* the following maybe use */
    char *dlmClusterNamePath;
    int mode;
}

static virLockManagerDlmDriverPtr driver = NULL;

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

    driver->requireLeaseForDisks = !driver->autoDiskLease;
    if (virCOnfGetValueBool(conf, "require_lease_for_disks", &driver->requireLeaseForDisks) < 0)
        goto cleanup;

    if (virConfGetValueString(conf, "locksapce_name", &driver->lockspaceName) < 0)
        goto cleanup;

    ret = 0;

 cleanup:
    virConfFree(conf);
    return ret;
}

static int virLockManagerDlmSetupLockspace(const char *path)
{
    virNetClientPtr client = NULL;
    virNetClientProgramPtr program = NULL;
    virLockSpaceProtocolCreateLockSpaceArgs args;
    int rv = -1;
    int counter = 0;

    memset(&args, 0, sizeof(args));
    args.path = path;

    if (!(client = virLockManagerDlmConnectNew(geteuid() == 0, &program)))
        return -1;

    // TODO
    if (virNetClientProgramCall(program,
                client,
                counter++,
                VIR_LOCK_SPACE_PROTOCOL_PROC_CREATE_LOCKSPACE,
                0, NULL, NULL, NULL,
                (xdrproc_t)xdr_virLockSpaceProtocolCreateLockSpaceArgs, (char *)&args,
                (xdrproc_t)xdr_void, NULL) < 0) {
        virErrorPtr err = virGetLastError();
        if (err && err->code == VIR_ERR_OPERATION_INVALID) {
            virResetLastError();
            rv = 0;
        } else {
            goto cleanup;
        }
    }

    rv = 0;

 cleanup:
    virObjectUnref(program);
    virNetClientClose(client);
    virObjectUnref(client);
    return rv;
}

static int virLockManagerDlmDeinit(void);

static int virLockManagerDlmInit(unsigned int version,
                                    const char *configFile,
                                    unsigned int flags)
{
    VIR_DEBUG("version=%u configFile=%s, flags=0x%x", version, NULLSTR(configFile), flags);

    virCheckFlags(0, -1);

    if (geteuid() != 0) {
        virReportError(VIR_ERR_INTERNAL_ERROR,
                       _("Dlm lock requires daemon has superuser privilege"));
        return -1;
    }

    if (driver)
        return 0;
    
    if (VIR_ALLOC(driver) < 0)
        return -1;

    driver->autoDiskLease = true;
    if (VIR_STRDUP(driver->lockspaceName, DLM_LOCKSPACE_NAME) < 0) {
        goto cleanup;
    }

    if (virLockManagerDlmLoadConfig(configFile) < 0)
        goto cleanup;

    if (driver->autoDiskLease) {
        if (driver->lockspaceName &&
                virLockManagerDlmSetupLockspace(driver->lockspaceName) < 0)
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

    VIR_FREE(driver->lockspaceName);
    VIR_FREE(driver);

    return 0;
}

static int virLockManagerDlmDaemonInquire(virLockManagerPtr lock ATTRIBUTE_UNUSED,
                                            char **state,
                                            unsigned int flags)
{
    // migration use it, set mannual lock such as sanlock
    virCheckFlags(0, -1);

    if (state)
        *state = NULL;

    return 0;
}

virLockDriver virLockDriverImpl =
{
    .version = VIR_LOCK_MANAGER_VERSION,
    .flags = 0,

    .drvNew = virLockManagerDlmNew;
    .drvFree = virLockManagerDlmFree;

    .drvAddResource = virLockManagerDlmAddResource,

    .drvAcquire = virLockManagerDlmAcquire,
    .drvRelease = virLockManagerDlmRelease,

    .drvInquire = virLockManagerDlmInquire,
}
