/*
 * lock_driver_dlm.c: a lock driver for dlm
 *
 * Copyright (C) 2018 SUSE LINUX Products, Beijing, China.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 */

#include <config.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <stdint.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>

#include <corosync/cpg.h>
#include <libdlm.h>

#include "lock_driver.h"
#include "viralloc.h"
#include "virconf.h"
#include "vircrypto.h"
#include "virerror.h"
#include "virfile.h"
#include "virhash.h"
#include "virlog.h"
#include "virstring.h"
#include "viruuid.h"

#include "configmake.h"

#define VIR_FROM_THIS VIR_FROM_LOCKING

#define VIR_RESOURCE_TABLE_SIZE 10

/* This will be set after dlm_controld is started. */
#define DLM_CLUSTER_NAME_PATH "/sys/kernel/config/dlm/cluster/cluster_name"

VIR_LOG_INIT("locking.lock_driver_dlm")

typedef struct _virLockManagerDLMLock virLockManagerDLMLock;
typedef virLockManagerDLMLock *virLockManagerDLMLockPtr;

typedef struct _virLockManagerDLMLockResource virLockManagerDLMLockResource;
typedef virLockManagerDLMLockResource *virLockManagerDLMLockResourcePtr;

typedef struct _virLockManagerDLMResource virLockManagerDLMResource;
typedef virLockManagerDLMResource *virLockManagerDLMResourcePtr;

typedef struct _virLockManagerDLMPrivate virLockManagerDLMPrivate;
typedef virLockManagerDLMPrivate *virLockManagerDLMPrivatePtr;

typedef struct _virLockManagerDLMDriver virLockManagerDLMDriver;
typedef virLockManagerDLMDriver *virLockManagerDLMDriverPtr;

struct _virLockManagerDLMLock {
    pid_t vm_pid;
    unsigned int lkid;
};

struct _virLockManagerDLMLockResource {
    char *name;
    unsigned int mode;
    size_t nHolders;
    size_t nLocks;
    virLockManagerDLMLockPtr locks;
};

struct _virLockManagerDLMResource {
    char *name;
    unsigned int mode;
};

struct _virLockManagerDLMPrivate {
    unsigned char vm_uuid[VIR_UUID_BUFLEN];
    char *vm_name;
    pid_t vm_pid;
    int vm_id;

    size_t nresources;
    virLockManagerDLMResourcePtr resources;

    bool hasRWDisks;
};

struct _virLockManagerDLMDriver {
    bool autoDiskLease;
    bool requireLeaseForDisks;

    bool purgeLockspace;
    char *lockspaceName;

    dlm_lshandle_t lockspace;
    virHashTablePtr resources;
    int lockFd;
};

static virLockManagerDLMDriverPtr driver;

static int virLockManagerDLMLoadConfig(const char *configFile)
{
    virConfPtr conf = NULL;
    int rv = -1;

    if (access(configFile, R_OK) == -1) {
        if (errno != ENOENT) {
            virReportSystemError(errno,
                                 _("unable to access config file %s"),
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
    if (virConfGetValueBool(conf, "require_lease_for_disks", &driver->requireLeaseForDisks) < 0)
        goto cleanup;

    if (virConfGetValueBool(conf, "purge_lockspace", &driver->purgeLockspace) < 0)
        goto cleanup;

    if (virConfGetValueString(conf, "lockspace_name", &driver->lockspaceName) < 0)
        goto cleanup;

    rv = 0;

 cleanup:
    virConfFree(conf);
    return rv;
}

static int
virLockManagerDLMWrite(virLockManagerDLMLockPtr lock, char *name)
{
    char *string = NULL;
    int rv = -1;

    if (virAsprintf(&string, "%u,%u,%s\n",
                    lock->lkid, (unsigned int)lock->vm_pid, name) < 0)
        goto cleanup;

    if (safewrite(driver->lockFd, string, strlen(string)) < 0)
        goto cleanup;

    if (fdatasync(driver->lockFd) < 0)
        goto cleanup;

    rv = 0;
 cleanup:
    VIR_FREE(string);

    return rv;
}

static int
virLockManagerDLMAdoptLocksInternal(void *payload,
                                    const void *name ATTRIBUTE_UNUSED,
                                    void *data ATTRIBUTE_UNUSED)
{
    virLockManagerDLMLockResourcePtr res = payload;
    unsigned int mode = LKM_PRMODE;
    struct dlm_lksb lksb;
    ssize_t i;
    int rv;

    for (i = res->nLocks-1; i >= 0; i--) {
        memset(&lksb, 0, sizeof(lksb));

        rv = dlm_ls_lockx(driver->lockspace, mode,
                          &lksb, LKF_PERSISTENT|LKF_ORPHAN,
                          res->name, strlen(res->name),
                          0, (void *)1, (void *)1,
                          (void *)1, NULL, NULL);
        if ((rv == -1) && (errno == EAGAIN)) {
            mode = LKM_EXMODE;
            rv = dlm_ls_lockx(driver->lockspace, mode,
                              &lksb, LKF_PERSISTENT|LKF_ORPHAN,
                              res->name, strlen(res->name),
                              0, (void *)1, (void *)1,
                              (void *)1, NULL, NULL);
        }

        if (rv < 0) {
            virReportError(VIR_ERR_INTERNAL_ERROR,
                           _("unable to adopt lock, rv=%d lockName=%s lockStatus=%d"),
                           rv, res->name, lksb.sb_status);
            VIR_DELETE_ELEMENT(res->locks, i, res->nLocks);

            continue;
        }

        res->locks[i].lkid = lksb.sb_lkid;
        res->nHolders += 1;
        res->mode = mode;
    }
    
    res->nLocks = res->nHolders;

    if (res->nLocks == 0)
        virHashRemoveEntry(driver->resources, res->name);

    return 0;
}

static void
virLockManagerDLMResourceDataFree(void *opaque,
                                  const void *name ATTRIBUTE_UNUSED)
{
    virLockManagerDLMLockResourcePtr res = opaque;
    virLockManagerDLMLockPtr lock;
    struct dlm_lksb lksb;
    int rv;

    if (!res)
        return;

    while(res->nLocks != 0) {
        lock = res->locks + res->nLocks - 1;
        rv = dlm_ls_unlock_wait(driver->lockspace, lock->lkid, 0, &lksb);
        if ((rv < 0) || (lksb.sb_status != EUNLOCK)) {
            VIR_WARN("unable to release lock: rv=%d lockStatus=%d",
                     rv, lksb.sb_status);
        }

        VIR_DELETE_ELEMENT(res->locks, res->nLocks-1, res->nLocks);
    }

    VIR_FREE(res->name);
    VIR_FREE(res);
}

static int
virLockManagerDLMAdoptLocks(const char *path)
{
    FILE *fp = NULL;
    char *line = NULL;
    char **tmpArray = NULL;
    virLockManagerDLMLockResourcePtr res = 0;
    unsigned int vm_pid = 0;
    unsigned int lkid = 0;
    size_t n = 0, tokcount = 0, i;
    ssize_t count = 0;
    int rv = -1;

    if (access(path, R_OK) != 0)
        return 0;

    fp = fopen(path, "r");
    if (fp == NULL) {
        virReportSystemError(errno,
                       _("unable to open '%s'"),
                       path);
        return -1;
    }

    while(!feof(fp)) {
        count = getline(&line, &n, fp);
        if (count <= 0)
            break;

        line[count-1] = '\0';

        if (!(tmpArray = virStringSplitCount(line, ",", 0, &tokcount)) ||
            (tokcount != 3))
            goto cleanup;

        if (virStrToLong_ui(tmpArray[0], NULL, 10, &lkid) < 0)
            goto cleanup;

        if (virStrToLong_ui(tmpArray[1], NULL, 10, &vm_pid) < 0)
            goto cleanup;

        if (!(res = virHashLookup(driver->resources, tmpArray[2]))) {
            if (VIR_ALLOC(res) < 0)
                goto cleanup;
            if (VIR_STRDUP(res->name, tmpArray[2]) < 0)
                goto cleanup;

            if (virHashAddEntry(driver->resources, res->name, res) < 0) {
                VIR_FREE(res->name);
                VIR_FREE(res);
                goto cleanup;
            }
        }

        if (vm_pid == 0) {
            for (i = 0; i < res->nLocks; i++) {
                if (lkid == res->locks[i].lkid) {
                    VIR_DELETE_ELEMENT(res->locks, i, res->nLocks);
                    break;
                }
            }
            continue;
        }

        if (VIR_EXPAND_N(res->locks, res->nLocks, 1) < 0)
            goto cleanup;

        res->locks[res->nLocks-1].vm_pid = vm_pid;
        res->locks[res->nLocks-1].lkid = lkid;

        virStringListFree(tmpArray);
    }

    if (virHashForEach(driver->resources,
                       virLockManagerDLMAdoptLocksInternal,
                       NULL) < 0)
        goto cleanup;

    rv = 0;

 cleanup:
    VIR_FORCE_FCLOSE(fp);
    return rv;
}

static int
virLockManagerDLMGetLocalNodeId(unsigned int *nodeId)
{
    cpg_handle_t handle = 0;
    int rv = -1;
    int result;

    result = cpg_model_initialize(&handle, CPG_MODEL_V1, NULL, NULL);

    virReportError(VIR_ERR_INTERNAL_ERROR, "result=%d, CS_OK=%d, cpg_handle=%ld, geteuid=%d", result, CS_OK, handle, geteuid());
    //if (cpg_model_initialize(&handle, CPG_MODEL_V1, NULL, NULL) != CS_OK) {
    if (result != CS_OK) {
        virReportError(VIR_ERR_INTERNAL_ERROR, "%s",
                       _("unable to create a new connection to the CPG service"));
        return -1;
    }

    if (cpg_local_get(handle, nodeId) != CS_OK) {
        virReportError(VIR_ERR_INTERNAL_ERROR, "%s",
                       _("unable to get the local node id by the CPG service"));
        goto cleanup;
    }

    VIR_DEBUG("the local nodeId=%u", *nodeId);

    rv = 0;
 cleanup:
    ignore_value(cpg_finalize(handle));

    return rv;
}

static int
virLockManagerDLMWriteLocksInternal(void *payload,
                                    const void *name ATTRIBUTE_UNUSED,
                                    void *data ATTRIBUTE_UNUSED)
{
    virLockManagerDLMLockResourcePtr res = payload;
    size_t i;

    for (i = 0; i < res->nLocks; i++) {
        if (virLockManagerDLMWrite(res->locks+i, res->name) < 0)
            return -1;
    }

    return 0;
}

static int
virLockManagerDLMSetupLockRecordFile(const bool newLockspace,
                                     const bool purgeLockspace)
{
    unsigned int nodeId = 0;
    char *path = NULL;
    int rv = -1;
    
    path = virFileBuildPath(RUNSTATEDIR, "/libvirt/DLMlocks", ".txt");

    if (!newLockspace &&
        virLockManagerDLMAdoptLocks(path) < 0) {
        goto cleanup;
    }

    if (purgeLockspace &&
        !virLockManagerDLMGetLocalNodeId(&nodeId)) {
        if (dlm_ls_purge(driver->lockspace, nodeId, 0) != 0) {
            VIR_WARN("node=%u purge DLM locks failed in lockspace=%s",
                     nodeId, driver->lockspaceName);
        }
        else
            VIR_DEBUG("node=%u purge DLM locks sucess in lockspace=%s",
                      nodeId, driver->lockspaceName);
    }

    driver->lockFd = open(path, O_WRONLY|O_CREAT|O_TRUNC, 0600);
    if (driver->lockFd < 0) {
        virReportSystemError(errno,
                             _("unable to open '%s'"),
                             path);
        goto cleanup;
    }

    if (virHashForEach(driver->resources,
                       virLockManagerDLMWriteLocksInternal,
                       NULL) < 0)
    goto cleanup;

    rv = 0;
 cleanup:
    VIR_FREE(path);

    return rv;
}

static int
virLockManagerDLMSetup(void)
{
    bool newLockspace = false;

    if (!(driver->resources = virHashCreate(VIR_RESOURCE_TABLE_SIZE,
                                            virLockManagerDLMResourceDataFree))) {
        virReportError(VIR_ERR_INTERNAL_ERROR, "%s",
                       _("unable to create resource hash table"));
        return -1;
    }

    if (!virFileExists(DLM_CLUSTER_NAME_PATH)) {
        virReportError(VIR_ERR_INTERNAL_ERROR, "%s",
                       _("check dlm_controld, ensure it has setuped"));
        return -1;
    }

    driver->lockspace = dlm_open_lockspace(driver->lockspaceName);
    if (!driver->lockspace) {
        driver->lockspace = dlm_create_lockspace(driver->lockspaceName, 0600);
        if (!driver->lockspace) {
            virReportSystemError(errno, "%s",
                                 _("unable to open and create DLM lockspace"));
            return -1;
        }
        newLockspace = true;
    }

    if (dlm_ls_pthread_init(driver->lockspace)) {
        if (errno != EEXIST) {
            virReportSystemError(errno, "%s",
                                 _("unable to initialize lockspace"));
            return -1;
        }
    }

    if (virLockManagerDLMSetupLockRecordFile(newLockspace,
                                             driver->purgeLockspace) < 0)  {
        return -1;
    }

    return 0;
} 

static int
virLockManagerDLMDeinit(void)
{
    if (!driver)
        return 0;

    if (driver->lockspace)
        ignore_value(dlm_close_lockspace(driver->lockspace));

    if (driver->resources)
        virHashFree(driver->resources);

    if (driver->lockFd)
        VIR_FORCE_CLOSE(driver->lockFd);

    VIR_FREE(driver->lockspaceName);
    VIR_FREE(driver);

    return 0;
}

static int
virLockManagerDLMInit(unsigned int version,
                      const char *configFile,
                      unsigned int flags)
{
    VIR_DEBUG("version=%u configFile=%s flags=0x%x", version, NULLSTR(configFile), flags);

    virCheckFlags(0, -1);

    if (driver)
        return 0;

    if (geteuid() != 0) {
        virReportError(VIR_ERR_INTERNAL_ERROR, "%s",
                       _("DLM lock requires root privileges"));
        return -1;
    }

    if (VIR_ALLOC(driver) < 0)
        return -1;

    driver->autoDiskLease = true;
    driver->requireLeaseForDisks = !driver->autoDiskLease;
    driver->purgeLockspace = true;

    if (VIR_STRDUP(driver->lockspaceName, "libvirt") < 0)
        goto error;

    if (virLockManagerDLMLoadConfig(configFile) < 0)
        goto error;

    if (virLockManagerDLMSetup() < 0)
        goto error;

    return 0;

 error:
    virLockManagerDLMDeinit();
    return -1;
}

static int
virLockManagerDLMNew(virLockManagerPtr lock,
                     unsigned int type,
                     size_t nparams,
                     virLockManagerParamPtr params,
                     unsigned int flags)
{
    virLockManagerDLMPrivatePtr priv = NULL;
    size_t i;

    virCheckFlags(VIR_LOCK_MANAGER_NEW_STARTED, -1);

    if (!driver) {
        virReportError(VIR_ERR_INTERNAL_ERROR, "%s",
                       _("DLM plugin is not initialized"));
        return -1;
    }

    if (type != VIR_LOCK_MANAGER_OBJECT_TYPE_DOMAIN) {
        virReportError(VIR_ERR_INTERNAL_ERROR,
                       _("unsupported object type %d"), type);
        return -1;
    }

    if (VIR_ALLOC(priv) < 0)
        return -1;

    lock->privateData = priv;

    for (i = 0; i < nparams; i++) {
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
            /* ignore */
        } else {
            virReportError(VIR_ERR_INTERNAL_ERROR,
                           _("unexpected parameter %s for object"),
                           params[i].key);
        }
    }

    if (priv->vm_pid == 0) {
        virReportError(VIR_ERR_INTERNAL_ERROR, "%s",
                       _("missing PID parameter for domain object"));
        return -1;
    }
    if (priv->vm_name == NULL) {
        virReportError(VIR_ERR_INTERNAL_ERROR, "%s",
                       _("missing name parameter for domain object"));
        return -1;
    }
    if (priv->vm_id == 0) {
        virReportError(VIR_ERR_INTERNAL_ERROR, "%s",
                       _("missing ID parameter for domain object"));
        return -1;
    }
    if (!virUUIDIsValid(priv->vm_uuid)) {
        virReportError(VIR_ERR_INTERNAL_ERROR, "%s",
                       _("missing UUID parameter for domain object"));
        return -1;
    }

    return 0;
}

static void
virLockManagerDLMFree(virLockManagerPtr lock)
{
    virLockManagerDLMPrivatePtr priv  = lock->privateData;

    if (!priv)
        return;

    VIR_FREE(priv->resources);
    VIR_FREE(priv->vm_name);
    VIR_FREE(priv);
    lock->privateData = NULL;

    return;
}

static int
virLockManagerDLMAddResource(virLockManagerPtr lock,
                             unsigned int type,
                             const char *name,
                             size_t nparams,
                             virLockManagerParamPtr params,
                             unsigned int flags)
{
    virLockManagerDLMPrivatePtr priv = lock->privateData;
    char *newName = NULL;
    int rv = -1;

    virCheckFlags(VIR_LOCK_MANAGER_RESOURCE_READONLY |
                  VIR_LOCK_MANAGER_RESOURCE_SHARED, -1);
 
    if (flags & VIR_LOCK_MANAGER_RESOURCE_READONLY)
        return 0;

    switch (type) {
    case VIR_LOCK_MANAGER_RESOURCE_TYPE_DISK:
        if (params || nparams) {
        virReportError(VIR_ERR_INTERNAL_ERROR, "%s",
                       _("unexpected parameters for disk resource"));
            return -1;
        }

        if (!driver->autoDiskLease) {
            if (!(flags & (VIR_LOCK_MANAGER_RESOURCE_SHARED |
                           VIR_LOCK_MANAGER_RESOURCE_READONLY))) {
                priv->hasRWDisks = true;

                return 0;
            }
        }

        if (virCryptoHashString(VIR_CRYPTO_HASH_SHA256, name, &newName) < 0)
            goto error;

        break;

    case VIR_LOCK_MANAGER_RESOURCE_TYPE_LEASE:
        if (VIR_STRDUP(newName, name) < 0)
            goto error;

        break;

    default:
        virReportError(VIR_ERR_INTERNAL_ERROR,
                       _("unknown lock manager object type %d"),
                       type);
        return -1;
    }

    if (VIR_EXPAND_N(priv->resources, priv->nresources, 1) < 0)
        goto error;

    VIR_STEAL_PTR(priv->resources[priv->nresources-1].name, newName);

    if (flags & VIR_LOCK_MANAGER_RESOURCE_SHARED)
        priv->resources[priv->nresources-1].mode = LKM_PRMODE;
    else
        priv->resources[priv->nresources-1].mode = LKM_EXMODE;

    rv = 0;
 error:
    VIR_FREE(newName);
    return rv;
}

static int
virLockManagerDLMAcquire(virLockManagerPtr lock,
                         const char *state ATTRIBUTE_UNUSED,
                         unsigned int flags,
                         virDomainLockFailureAction action ATTRIBUTE_UNUSED,
                         int *fd)
{
    virLockManagerDLMPrivatePtr priv = lock->privateData;
    virLockManagerDLMResourcePtr args = NULL;
    virLockManagerDLMLockResourcePtr res = NULL;
    struct dlm_lksb lksb;
    int rv = -1;
    size_t i, index;

    virCheckFlags(VIR_LOCK_MANAGER_ACQUIRE_REGISTER_ONLY |
                  VIR_LOCK_MANAGER_ACQUIRE_RESTRICT, -1);

    if (priv->nresources == 0 &&
        priv->hasRWDisks &&
        driver->requireLeaseForDisks) {
        virReportError(VIR_ERR_CONFIG_UNSUPPORTED, "%s",
                       _("read/write, exclusive access, disk were present, but no leases specified"));
        return -1;
    }

    if (fd)
        *fd = -1;

    if (!driver->lockspace) {
        virReportError(VIR_ERR_INTERNAL_ERROR, "%s",
                       _("lockspace is not opened"));
        return -1;
    }

    if (!(flags & VIR_LOCK_MANAGER_ACQUIRE_REGISTER_ONLY)) {
        VIR_DEBUG("Acquiring object %zu", priv->nresources);

        for (i = 0; i < priv->nresources; i++) {
            args = priv->resources + i;
            memset(&lksb, 0, sizeof(lksb));

            if (!(res = virHashLookup(driver->resources, args->name))) {
                if (VIR_ALLOC(res) < 0)
                    return -1;
                if (VIR_STRDUP(res->name, args->name) < 0)
                    return -1;

                if (virHashAddEntry(driver->resources, args->name, res) < 0) {
                    VIR_FREE(res->name);
                    VIR_FREE(res);
                    return -1;
                }
            }

            if (res->nLocks == res->nHolders) {
                rv = dlm_ls_lock_wait(driver->lockspace, LKM_NLMODE,
                                      &lksb, LKF_EXPEDITE,
                                      args->name, strlen(args->name),
                                      0, NULL, NULL, NULL);
                if ((rv < 0) || (lksb.sb_status != 0)) {
                    virReportError(VIR_ERR_INTERNAL_ERROR,
                                   _("unable to add NL lock, rv=%d, lockStatus=%d"),
                                   rv, lksb.sb_status);
                    return -1;
                }
                if (VIR_EXPAND_N(res->locks, res->nLocks, 1) < 0)
                    return -1;
            
                index = res->nLocks - 1;
                res->locks[index].vm_pid = 0;
                res->locks[index].lkid = lksb.sb_lkid;
            }
            else {
                for (index = 0; index < res->nLocks; index++) {
                    if (res->locks[index].vm_pid == 0)
                        break;
                } 
                lksb.sb_lkid = res->locks[index].lkid;
            }

            rv = dlm_ls_lock_wait(driver->lockspace, args->mode,
                                  &lksb, LKF_CONVERT|LKF_NOQUEUE|LKF_PERSISTENT,
                                  args->name, strlen(args->name),
                                  0, NULL, NULL, NULL);
            if ((rv < 0) || (lksb.sb_status != 0)) {
                if (lksb.sb_status == EAGAIN)
                    virReportError(VIR_ERR_INTERNAL_ERROR, "%s",
                                   _("failed to acquire lock: the lock could not be granted"));
                else
                    virReportError(errno,
                                   _("failed to acquire lock: rv=%d lockStatus=%d"),
                                   rv, lksb.sb_status);
                return -1;
            }

            res->locks[index].vm_pid = priv->vm_pid;
            res->nHolders += 1;
            res->mode = args->mode;

            if (virLockManagerDLMWrite(res->locks+index, res->name) < 0) {
                virReportSystemError(errno, "%s",
                                     "unable to write lock information to file");
                return -1;
            }
        }
    }

    if (flags & VIR_LOCK_MANAGER_ACQUIRE_RESTRICT) {
        ignore_value(dlm_close_lockspace(driver->lockspace));
        driver->lockspace = NULL;
    }

    return 0;
}

static int
virLockManagerDLMRelease(virLockManagerPtr lock,
                         char **state,
                         unsigned int flags)
{
    virLockManagerDLMPrivatePtr priv = lock->privateData;
    virLockManagerDLMResourcePtr args = NULL;
    virLockManagerDLMLockResourcePtr res = NULL;
    struct dlm_lksb lksb;
    int rv = -1;
    size_t i;

    virCheckFlags(0, -1);

    if (state)
        *state = NULL;

    if (!driver->lockspace) {
        virReportError(VIR_ERR_INTERNAL_ERROR, "%s",
                       _("lockspace is not opened"));
        return -1;
    }

    for (i = 0; i < priv->nresources; i++) {
        args = priv->resources + i;

        if (!(res = virHashLookup(driver->resources, args->name)))
            continue;

        if (res->nHolders == 0)
            continue;

        for (i = 0; i < res->nLocks; i++) {
            if (priv->vm_pid == res->locks[i].vm_pid)
                break;
        }

        if (i == res->nLocks)
            continue;

        memset(&lksb, 0, sizeof(lksb));
        lksb.sb_lkid = res->locks[i].lkid;
        rv = dlm_ls_lock_wait(driver->lockspace, LKM_NLMODE,
                              &lksb, LKF_CONVERT,
                              res->name, strlen(res->name), 
                              0, NULL, NULL, NULL);
        if ((rv < 0) || (lksb.sb_status != 0)) {
            virReportError(VIR_ERR_INTERNAL_ERROR,
                           _("failed to release lock: rv=%d lockStatus=%d"),
                           rv, lksb.sb_status);
            goto cleanup;
        }

        res->nHolders -= 1;
        res->locks[i].vm_pid = 0;
        if (virLockManagerDLMWrite(res->locks+i, res->name) < 0) {
            virReportSystemError(errno, "%s",
                                 "unable to write lock information to file");
            return -1;
        }

        if (res->nHolders == 0)
            virHashRemoveEntry(driver->resources, res->name);

    }

    rv = 0;
 cleanup:
    return rv;
}

static int
virLockManagerDLMInquire(virLockManagerPtr lock ATTRIBUTE_UNUSED,
                         char **state,
                         unsigned int flags)
{
    virCheckFlags(0, -1);

    if (state)
        *state = NULL;

    return 0;
}

virLockDriver virLockDriverImpl =
{
    .version = VIR_LOCK_MANAGER_VERSION,

    .flags = VIR_LOCK_MANAGER_USES_STATE,

    .drvInit = virLockManagerDLMInit,
    .drvDeinit = virLockManagerDLMDeinit,

    .drvNew = virLockManagerDLMNew,
    .drvFree = virLockManagerDLMFree,

    .drvAddResource = virLockManagerDLMAddResource,

    .drvAcquire = virLockManagerDLMAcquire,
    .drvRelease = virLockManagerDLMRelease,
    .drvInquire = virLockManagerDLMInquire,
};
