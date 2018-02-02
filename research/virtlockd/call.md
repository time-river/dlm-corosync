
// remove any handles that were previously marked as deleted
virEventRunDefaultImpl
  virEventPollRunOnce
    virEventPollCleanupHandles
      ff(opaque) --> virNetSocketEventFree
        ff(eopaque) --> virObjectUnref
          klass->dispose(obj) --> virNetServerClientDispose
            client->privateDataFreeFunc(client->privateData) --> virLockDaemonClientFree
