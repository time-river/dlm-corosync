/**
 * header: include/libvirt/libvirt-domain.h
 */
typedef enum {
    VIR_DOMAIN_NOSTATE = 0,     /* no state */
    VIR_DOMAIN_RUNNING = 1,     /* the domain is running */
    VIR_DOMAIN_BLOCKED = 2,     /* the domain is blocked on resource */
    VIR_DOMAIN_PAUSED  = 3,     /* the domain is paused by user */
    VIR_DOMAIN_SHUTDOWN= 4,     /* the domain is being shut down */
    VIR_DOMAIN_SHUTOFF = 5,     /* the domain is shut off */
    VIR_DOMAIN_CRASHED = 6,     /* the domain is crashed */
    VIR_DOMAIN_PMSUSPENDED = 7, /* the domain is suspended by guest
                                   power management */

# ifdef VIR_ENUM_SENTINELS
    VIR_DOMAIN_LAST
    /*
     * NB: this enum value will increase over time as new events are
     * added to the libvirt API. It reflects the last state supported
     * by this version of the libvirt API.
     */
# endif
} virDomainState;


/**
 * header: src/driver-hypervisor.h
 * qemu implementation: src/qemu/qemu_driver.c
 */
static virHypervisorDriver qemuHypervisorDriver = {
    .domainCreateXML = qemuDomainCreateXML, // create, decide vCPUs start or not according args, set state `VIR_DOMAIN_PAUSED` or `VIR_DOMAIN_RUNNING`
                                        virDomainLockProcessStart
    .domainSuspend = qemuDomainSuspend, // suspend vCPUs, set state `VIR_DOMAIN_PAUSED`
                                        virDomainLockProcessPause
    .domainResume = qemuDomainResume,  // virDomainLockProcessResume, set state `VIR_DOMAIN_RUNNING`
                                        virDomainLockProcessResume
    .domainShutdown = qemuDomainShutdown,
    .domainShutdownFlags = qemuDomainShutdownFlags,
    
    .domainReboot = qemuDomainReboot, 

    .domainReset = qemuDomainReset,

    domainCreate = qemuDomainCreate,
    .domainCreateWithFlags = qemuDomainCreateWithFlags, // virsh start
}

/**
 * virDomainCreateXML:
 * @conn: pointer to the hypervisor connection
 * @xmlDesc: string containing an XML description of the domain
 * @flags: bitwise-OR of supported virDomainCreateFlags
 *
 * Launch a new guest domain, based on an XML description similar
 * to the one returned by virDomainGetXMLDesc()
 * This function may require privileged access to the hypervisor.
 * The domain is not persistent, so its definition will disappear when it
 * is destroyed, or if the host is restarted (see virDomainDefineXML() to
 * define persistent domains).
 *
 * If the VIR_DOMAIN_START_PAUSED flag is set, the guest domain
 * will be started, but its CPUs will remain paused. The CPUs
 * can later be manually started using virDomainResume.
 *
 * If the VIR_DOMAIN_START_AUTODESTROY flag is set, the guest
 * domain will be automatically destroyed when the virConnectPtr
 * object is finally released. This will also happen if the
 * client application crashes / loses its connection to the
 * libvirtd daemon. Any domains marked for auto destroy will
 * block attempts at migration, save-to-file, or snapshots.
 *
 * virDomainFree should be used to free the resources after the
 * domain object is no longer needed.
 *
 * Returns a new domain object or NULL in case of failure
 */
static virDomainPtr qemuDomainCreateXML(virConnectPtr conn,
                                    const char *xml,
                                    unsigned int flags)
    qemuProcessStart(conn, driver, vm, NULL, QEMU_ASYNC_JOB_START,
            NULL, -1, NULL, NULL,
            VIR_NETDEV_VPORT_PROFILE_OP_CREATE,
            start_flags) < 0)
        qemuProcessInit(driver, vm, updatedCPU, // not execute virCommandRun
                asyncJob, !!migrateFrom, flags)
            qemuProcessStartHook(driver, vm,
                    VIR_HOOK_QEMU_OP_PREPARE,
                    VIR_HOOK_SUBOP_BEGIN)
                ...
        qemuProcessLaunch(conn, driver, vm, asyncJob, incoming,
                    snapshot, vmop, flags)
            qemuProcessStartHook(driver, vm,
                        VIR_HOOK_QEMU_OP_START,
                        VIR_HOOK_SUBOP_BEGIN)
                virHookCall(VIR_HOOK_DRIVER_QEMU, vm->def->name, op, subop, // not execute virCommandRun
                            NULL, xml, NULL)
            virCommandRun(cmd, NULL)
                        virCommandRunAsync(cmd, NULL)
                            fork // maybe not start
                            for child: lock, then exec, start qemu
                            for parent: virThreadCreate(cmd->asyncioThread, true, virCommandDoAsyncIOHelper, cmd)
                                            virCommandProcessIO(cmd) // Manage input and output to the child process
                                                poll(fds, nfds, -1)
        qemuProcessFinishStartup(conn, driver, vm, asyncJob,
                                !(flags & VIR_QEMU_PROCESS_START_PAUSED),
                                incoming ?
                                VIR_DOMAIN_PAUSED_MIGRATION :
                                VIR_DOMAIN_PAUSED_USER)
            qemuProcessStartCPUs(driver, vm, conn,
                                 VIR_DOMAIN_RUNNING_BOOTED,
                                 asyncJob) // asyncJob == 1
