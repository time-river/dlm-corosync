2018-02-01 03:21:20.571+0000: 11815: debug : virEventPollMakePollFDs:401 : Prepare n=0 w=1, f=4 e=1 d=0
2018-02-01 03:21:20.571+0000: 11815: debug : virEventPollMakePollFDs:401 : Prepare n=1 w=2, f=6 e=1 d=0
2018-02-01 03:21:20.571+0000: 11815: debug : virEventPollMakePollFDs:401 : Prepare n=2 w=3, f=9 e=1 d=0
2018-02-01 03:21:20.571+0000: 11815: debug : virEventPollMakePollFDs:401 : Prepare n=3 w=4, f=10 e=1 d=0
2018-02-01 03:21:20.571+0000: 11815: debug : virEventPollMakePollFDs:401 : Prepare n=4 w=5, f=11 e=1 d=0
2018-02-01 03:21:20.571+0000: 11815: debug : virEventPollMakePollFDs:401 : Prepare n=5 w=6, f=12 e=1 d=0
2018-02-01 03:21:20.571+0000: 11815: debug : virEventPollMakePollFDs:401 : Prepare n=6 w=7, f=14 e=1 d=0
2018-02-01 03:21:20.571+0000: 11815: debug : virEventPollMakePollFDs:401 : Prepare n=7 w=8, f=13 e=0 d=0
2018-02-01 03:21:20.571+0000: 11815: debug : virEventPollMakePollFDs:401 : Prepare n=8 w=9, f=13 e=1 d=0
2018-02-01 03:21:20.571+0000: 11815: debug : virEventPollMakePollFDs:401 : Prepare n=9 w=10, f=19 e=1 d=0
2018-02-01 03:21:20.571+0000: 11815: debug : virEventPollMakePollFDs:401 : Prepare n=10 w=12, f=24 e=1 d=0
2018-02-01 03:21:20.571+0000: 11815: debug : virEventPollMakePollFDs:401 : Prepare n=11 w=13, f=25 e=1 d=0
2018-02-01 03:21:20.571+0000: 11815: debug : virEventPollMakePollFDs:401 : Prepare n=12 w=15, f=28 e=25 d=0
2018-02-01 03:21:20.571+0000: 11815: debug : virEventPollCalculateTimeout:338 : Calculate expiry of 2 timers
2018-02-01 03:21:20.571+0000: 11815: debug : virEventPollCalculateTimeout:346 : Got a timeout scheduled for 1517455284927
2018-02-01 03:21:20.571+0000: 11815: debug : virEventPollCalculateTimeout:359 : Schedule timeout then=1517455284927 now=1517455280571
2018-02-01 03:21:20.571+0000: 11815: debug : virEventPollCalculateTimeout:369 : Timeout at 1517455284927 due in 4356 ms
2018-02-01 03:21:20.571+0000: 11815: info : virEventPollRunOnce:640 : EVENT_POLL_RUN: nhandles=12 timeout=4356
2018-02-01 03:21:20.599+0000: 11815: debug : virEventPollRunOnce:650 : Poll got 1 event(s)
2018-02-01 03:21:20.599+0000: 11815: debug : virEventPollDispatchTimeouts:432 : Dispatch 2
2018-02-01 03:21:20.599+0000: 11815: debug : virEventPollDispatchHandles:478 : Dispatch 12
2018-02-01 03:21:20.599+0000: 11815: debug : virEventPollDispatchHandles:492 : i=0 w=1
2018-02-01 03:21:20.599+0000: 11815: debug : virEventPollDispatchHandles:492 : i=1 w=2
2018-02-01 03:21:20.599+0000: 11815: debug : virEventPollDispatchHandles:492 : i=2 w=3
2018-02-01 03:21:20.599+0000: 11815: debug : virEventPollDispatchHandles:492 : i=3 w=4
2018-02-01 03:21:20.599+0000: 11815: debug : virEventPollDispatchHandles:492 : i=4 w=5
2018-02-01 03:21:20.599+0000: 11815: debug : virEventPollDispatchHandles:492 : i=5 w=6
2018-02-01 03:21:20.599+0000: 11815: debug : virEventPollDispatchHandles:492 : i=6 w=7
2018-02-01 03:21:20.599+0000: 11815: debug : virEventPollDispatchHandles:492 : i=8 w=9
2018-02-01 03:21:20.599+0000: 11815: debug : virEventPollDispatchHandles:492 : i=9 w=10
2018-02-01 03:21:20.599+0000: 11815: debug : virEventPollDispatchHandles:492 : i=10 w=12
2018-02-01 03:21:20.599+0000: 11815: debug : virEventPollDispatchHandles:492 : i=11 w=13
2018-02-01 03:21:20.599+0000: 11815: debug : virEventPollDispatchHandles:492 : i=12 w=15
2018-02-01 03:21:20.599+0000: 11815: info : virEventPollDispatchHandles:506 : EVENT_POLL_DISPATCH_HANDLE: watch=15 events=9
2018-02-01 03:21:20.599+0000: 11815: info : virObjectRef:365 : OBJECT_REF: obj=0x7fe4080163d0
2018-02-01 03:21:20.599+0000: 11815: debug : qemuMonitorIO:754 : Error on monitor <null>
2018-02-01 03:21:20.599+0000: 11815: info : virEventPollUpdateHandle:152 : EVENT_POLL_UPDATE_HANDLE: watch=15 events=13
2018-02-01 03:21:20.599+0000: 11815: debug : virEventPollInterruptLocked:722 : Skip interrupt, 1 140617919043904
2018-02-01 03:21:20.599+0000: 11815: debug : qemuMonitorIO:775 : Triggering EOF callback
2018-02-01 03:21:20.599+0000: 11815: debug : qemuProcessHandleMonitorEOF:289 : Received EOF on 0x7fe3bc185f50 'generic'
2018-02-01 03:21:20.599+0000: 11815: info : virObjectRef:365 : OBJECT_REF: obj=0x7fe3bc185f50
2018-02-01 03:21:20.599+0000: 12114: debug : qemuProcessEventHandler:4740 : vm=0x7fe3bc185f50, event=6
2018-02-01 03:21:20.599+0000: 11815: info : virEventPollRemoveHandle:186 : EVENT_POLL_REMOVE_HANDLE: watch=15
2018-02-01 03:21:20.599+0000: 11815: debug : virEventPollRemoveHandle:199 : mark delete 12 28
2018-02-01 03:21:20.599+0000: 11815: debug : virEventPollInterruptLocked:722 : Skip interrupt, 1 140617919043904
2018-02-01 03:21:20.599+0000: 12114: debug : qemuProcessKill:5967 : vm=0x7fe3bc185f50 name=generic pid=12700 flags=1
2018-02-01 03:21:20.599+0000: 12114: debug : virProcessKillPainfully:355 : vpid=12700 force=1
2018-02-01 03:21:20.599+0000: 11815: info : virObjectUnref:327 : OBJECT_UNREF: obj=0x7fe4080163d0
2018-02-01 03:21:20.599+0000: 11815: debug : virEventPollCleanupTimeouts:525 : Cleanup 2
2018-02-01 03:21:20.599+0000: 11815: debug : virEventPollCleanupHandles:574 : Cleanup 13
2018-02-01 03:21:20.599+0000: 11815: info : virEventPollCleanupHandles:587 : EVENT_POLL_PURGE_HANDLE: watch=15
2018-02-01 03:21:20.599+0000: 11815: info : virObjectUnref:327 : OBJECT_UNREF: obj=0x7fe4080163d0
2018-02-01 03:21:20.599+0000: 11815: debug : virEventRunDefaultImpl:311 : running default event implementation
2018-02-01 03:21:20.599+0000: 11815: debug : virEventPollCleanupTimeouts:525 : Cleanup 2
2018-02-01 03:21:20.599+0000: 11815: debug : virEventPollCleanupHandles:574 : Cleanup 12
2018-02-01 03:21:20.599+0000: 11815: debug : virEventPollMakePollFDs:401 : Prepare n=0 w=1, f=4 e=1 d=0
2018-02-01 03:21:20.599+0000: 11815: debug : virEventPollMakePollFDs:401 : Prepare n=1 w=2, f=6 e=1 d=0
2018-02-01 03:21:20.599+0000: 11815: debug : virEventPollMakePollFDs:401 : Prepare n=2 w=3, f=9 e=1 d=0
2018-02-01 03:21:20.599+0000: 11815: debug : virEventPollMakePollFDs:401 : Prepare n=3 w=4, f=10 e=1 d=0
2018-02-01 03:21:20.599+0000: 11815: debug : virEventPollMakePollFDs:401 : Prepare n=4 w=5, f=11 e=1 d=0
2018-02-01 03:21:20.599+0000: 11815: debug : virEventPollMakePollFDs:401 : Prepare n=5 w=6, f=12 e=1 d=0
2018-02-01 03:21:20.599+0000: 11815: debug : virEventPollMakePollFDs:401 : Prepare n=6 w=7, f=14 e=1 d=0
2018-02-01 03:21:20.599+0000: 11815: debug : virEventPollMakePollFDs:401 : Prepare n=7 w=8, f=13 e=0 d=0
2018-02-01 03:21:20.599+0000: 11815: debug : virEventPollMakePollFDs:401 : Prepare n=8 w=9, f=13 e=1 d=0
2018-02-01 03:21:20.599+0000: 11815: debug : virEventPollMakePollFDs:401 : Prepare n=9 w=10, f=19 e=1 d=0
2018-02-01 03:21:20.599+0000: 11815: debug : virEventPollMakePollFDs:401 : Prepare n=10 w=12, f=24 e=1 d=0
2018-02-01 03:21:20.599+0000: 11815: debug : virEventPollMakePollFDs:401 : Prepare n=11 w=13, f=25 e=1 d=0
2018-02-01 03:21:20.599+0000: 11815: debug : virEventPollCalculateTimeout:338 : Calculate expiry of 2 timers
2018-02-01 03:21:20.599+0000: 11815: debug : virEventPollCalculateTimeout:346 : Got a timeout scheduled for 1517455284927
2018-02-01 03:21:20.599+0000: 11815: debug : virEventPollCalculateTimeout:359 : Schedule timeout then=1517455284927 now=1517455280599
2018-02-01 03:21:20.599+0000: 11815: debug : virEventPollCalculateTimeout:369 : Timeout at 1517455284927 due in 4328 ms
2018-02-01 03:21:20.599+0000: 11815: info : virEventPollRunOnce:640 : EVENT_POLL_RUN: nhandles=11 timeout=4328
2018-02-01 03:21:20.799+0000: 12114: info : virObjectRef:365 : OBJECT_REF: obj=0x7fe3bc24e5a0
2018-02-01 03:21:20.799+0000: 12114: debug : qemuDomainObjBeginJobInternal:3846 : Starting job: destroy (vm=0x7fe3bc185f50 name=generic, current job=none async=none)
2018-02-01 03:21:20.799+0000: 12114: debug : qemuDomainObjBeginJobInternal:3887 : Started job: destroy (async=none vm=0x7fe3bc185f50 name=generic)
2018-02-01 03:21:20.799+0000: 12114: info : virObjectRef:365 : OBJECT_REF: obj=0x7fe3bc24e5a0
2018-02-01 03:21:20.800+0000: 12114: debug : virFileMakePathHelper:2912 : path=/var/run/libvirt/qemu mode=0777
2018-02-01 03:21:20.800+0000: 12114: debug : virFileClose:110 : Closed fd 17
2018-02-01 03:21:20.800+0000: 12114: info : virObjectUnref:327 : OBJECT_UNREF: obj=0x7fe3bc24e5a0
2018-02-01 03:21:20.800+0000: 12114: info : virObjectUnref:327 : OBJECT_UNREF: obj=0x7fe3bc24e5a0
2018-02-01 03:21:20.800+0000: 12114: info : virObjectNew:231 : OBJECT_NEW: obj=0x7fe3bc26b900 classname=virDomainEventLifecycle
2018-02-01 03:21:20.800+0000: 12114: debug : virObjectEventNew:645 : obj=0x7fe3bc26b900
2018-02-01 03:21:20.800+0000: 12114: info : virObjectRef:365 : OBJECT_REF: obj=0x7fe3bc24e5a0
2018-02-01 03:21:20.800+0000: 12114: debug : qemuProcessStop:6051 : Shutting down vm=0x7fe3bc185f50 name=generic id=8 pid=12700, reason=shutdown, asyncJob=none, flags=0
2018-02-01 03:21:20.800+0000: 12114: info : virObjectRef:365 : OBJECT_REF: obj=0x7fe3bc24e5a0
2018-02-01 03:21:20.800+0000: 12114: info : virObjectUnref:327 : OBJECT_UNREF: obj=0x7fe3bc24e5a0
2018-02-01 03:21:20.800+0000: 12114: debug : virNetDaemonRemoveShutdownInhibition:567 : dmn=0x55d686a622b0 inhibitions=0
2018-02-01 03:21:20.800+0000: 12114: debug : virNetDaemonRemoveShutdownInhibition:570 : Closing inhibit FD 23
2018-02-01 03:21:20.800+0000: 12114: debug : virFileClose:110 : Closed fd 23
2018-02-01 03:21:20.800+0000: 12114: info : virObjectRef:365 : OBJECT_REF: obj=0x7fe3bc24e5a0
2018-02-01 03:21:20.800+0000: 12114: debug : qemuDomainLogAppendMessage:4876 : Append log message (vm='generic' message='2018-02-01 03:21:20.800+0000: shutting down, reason=shutdown
) stdioLogD=1
2018-02-01 03:21:20.800+0000: 12114: debug : virNetSocketNewConnectUNIX:640 : path=/var/run/libvirt/virtlogd-sock spawnDaemon=0 binary=<null>
2018-02-01 03:21:20.800+0000: 12114: debug : virNetSocketNewConnectUNIX:704 : connect() succeeded
2018-02-01 03:21:20.800+0000: 12114: debug : virNetSocketNew:236 : localAddr=0x7fe3ce7fb800 remoteAddr=0x7fe3ce7fb890 fd=17 errfd=-1 pid=0
2018-02-01 03:21:20.800+0000: 12114: info : virObjectNew:231 : OBJECT_NEW: obj=0x7fe3bc2140e0 classname=virNetSocket
2018-02-01 03:21:20.800+0000: 12114: info : virNetSocketNew:292 : RPC_SOCKET_NEW: sock=0x7fe3bc2140e0 fd=17 errfd=-1 pid=0 localAddr=127.0.0.1;0, remoteAddr=127.0.0.1;0
2018-02-01 03:21:20.800+0000: 12114: info : virObjectNew:231 : OBJECT_NEW: obj=0x7fe3bc2d2880 classname=virNetClient
2018-02-01 03:21:20.800+0000: 12114: info : virNetClientNew:328 : RPC_CLIENT_NEW: client=0x7fe3bc2d2880 sock=0x7fe3bc2140e0
2018-02-01 03:21:20.800+0000: 12114: info : virObjectNew:231 : OBJECT_NEW: obj=0x7fe3bc267a00 classname=virNetClientProgram
2018-02-01 03:21:20.800+0000: 12114: info : virObjectRef:365 : OBJECT_REF: obj=0x7fe3bc267a00
2018-02-01 03:21:20.800+0000: 12114: debug : virNetMessageNew:46 : msg=0x7fe3bc258950 tracked=0
2018-02-01 03:21:20.800+0000: 12114: debug : virNetMessageEncodePayload:387 : Encode length as 176
2018-02-01 03:21:20.800+0000: 12114: info : virNetClientSendInternal:2120 : RPC_CLIENT_MSG_TX_QUEUE: client=0x7fe3bc2d2880 len=176 prog=2270401305 vers=1 proc=4 type=0 status=0 serial=0
2018-02-01 03:21:20.800+0000: 12114: debug : virNetClientCallNew:2073 : New call 0x7fe3bc2d0340: msg=0x7fe3bc258950, expectReply=1, nonBlock=0
2018-02-01 03:21:20.800+0000: 12114: debug : virNetClientIO:1879 : Outgoing message prog=2270401305 version=1 serial=0 proc=4 type=0 length=176 dispatch=(nil)
2018-02-01 03:21:20.800+0000: 12114: debug : virNetClientIO:1938 : We have the buck head=0x7fe3bc2d0340 call=0x7fe3bc2d0340
2018-02-01 03:21:20.801+0000: 12114: debug : virNetSocketUpdateIOCallback:2199 : Watch not registered on socket 0x7fe3bc2140e0
2018-02-01 03:21:20.802+0000: 12114: debug : virNetMessageDecodeLength:161 : Got length, now need 32 total (28 more)
2018-02-01 03:21:20.802+0000: 12114: info : virNetClientCallDispatch:1267 : RPC_CLIENT_MSG_RX: client=0x7fe3bc2d2880 len=32 prog=2270401305 vers=1 proc=4 type=1 status=0 serial=0
2018-02-01 03:21:20.802+0000: 12114: debug : virKeepAliveCheckMessage:374 : ka=(nil), client=0x7fe428bb71f9, msg=0x7fe3bc2d28e8
2018-02-01 03:21:20.802+0000: 12114: debug : virNetMessageClear:74 : msg=0x7fe3bc2d28e8 nfds=0
2018-02-01 03:21:20.802+0000: 12114: debug : virNetClientIOEventLoopPassTheBuck:1562 : Giving up the buck 0x7fe3bc2d0340
2018-02-01 03:21:20.802+0000: 12114: debug : virNetClientIOEventLoopPassTheBuck:1576 : No thread to pass the buck to
2018-02-01 03:21:20.802+0000: 12114: debug : virNetSocketUpdateIOCallback:2199 : Watch not registered on socket 0x7fe3bc2140e0
2018-02-01 03:21:20.802+0000: 12114: debug : virNetClientIO:1968 : All done with our call head=(nil) call=0x7fe3bc2d0340 rv=0
2018-02-01 03:21:20.802+0000: 12114: debug : virNetMessageFree:87 : msg=0x7fe3bc258950 nfds=0 cb=(nil)
2018-02-01 03:21:20.802+0000: 12114: debug : virNetClientCloseInternal:840 : client=0x7fe3bc2d2880 wantclose=0
2018-02-01 03:21:20.802+0000: 12114: debug : virNetClientMarkClose:776 : client=0x7fe3bc2d2880, reason=3
2018-02-01 03:21:20.802+0000: 12114: debug : virNetSocketRemoveIOCallback:2214 : Watch not registered on socket 0x7fe3bc2140e0
2018-02-01 03:21:20.802+0000: 12114: debug : virNetClientIOEventLoopPassTheBuck:1562 : Giving up the buck (nil)
2018-02-01 03:21:20.802+0000: 12114: debug : virNetClientIOEventLoopPassTheBuck:1576 : No thread to pass the buck to
2018-02-01 03:21:20.802+0000: 12114: debug : virNetClientCloseLocked:796 : client=0x7fe3bc2d2880, sock=0x7fe3bc2140e0, reason=3
2018-02-01 03:21:20.802+0000: 12114: info : virObjectUnref:327 : OBJECT_UNREF: obj=0x7fe3bc2140e0
2018-02-01 03:21:20.802+0000: 12114: info : virObjectUnref:329 : OBJECT_DISPOSE: obj=0x7fe3bc2140e0
2018-02-01 03:21:20.802+0000: 12114: info : virNetSocketDispose:1324 : RPC_SOCKET_DISPOSE: sock=0x7fe3bc2140e0
2018-02-01 03:21:20.802+0000: 12114: debug : virFileClose:110 : Closed fd 17
2018-02-01 03:21:20.802+0000: 12114: info : virObjectUnref:327 : OBJECT_UNREF: obj=0x7fe3bc267a00
2018-02-01 03:21:20.802+0000: 12114: info : virObjectUnref:327 : OBJECT_UNREF: obj=0x7fe3bc2d2880
2018-02-01 03:21:20.802+0000: 12114: info : virObjectUnref:329 : OBJECT_DISPOSE: obj=0x7fe3bc2d2880
2018-02-01 03:21:20.803+0000: 12114: info : virNetClientDispose:744 : RPC_CLIENT_DISPOSE: client=0x7fe3bc2d2880
2018-02-01 03:21:20.803+0000: 12114: info : virObjectUnref:327 : OBJECT_UNREF: obj=0x7fe3bc267a00
2018-02-01 03:21:20.803+0000: 12114: info : virObjectUnref:329 : OBJECT_DISPOSE: obj=0x7fe3bc267a00
2018-02-01 03:21:20.803+0000: 12114: debug : virFileClose:110 : Closed fd 26
2018-02-01 03:21:20.803+0000: 12114: debug : virFileClose:110 : Closed fd 23
2018-02-01 03:21:20.803+0000: 12114: debug : virNetMessageClear:74 : msg=0x7fe3bc2d28e8 nfds=0
2018-02-01 03:21:20.803+0000: 12114: info : virObjectUnref:327 : OBJECT_UNREF: obj=0x7fe3bc24e5a0
2018-02-01 03:21:20.803+0000: 12114: info : qemuMonitorClose:993 : QEMU_MONITOR_CLOSE: mon=0x7fe4080163d0 refs=1
2018-02-01 03:21:20.803+0000: 12114: debug : virFileClose:110 : Closed fd 28
2018-02-01 03:21:20.803+0000: 12114: info : virObjectUnref:327 : OBJECT_UNREF: obj=0x7fe4080163d0
2018-02-01 03:21:20.803+0000: 12114: info : virObjectUnref:329 : OBJECT_DISPOSE: obj=0x7fe4080163d0
2018-02-01 03:21:20.803+0000: 12114: debug : qemuMonitorDispose:323 : mon=0x7fe4080163d0
2018-02-01 03:21:20.803+0000: 12114: info : virObjectUnref:327 : OBJECT_UNREF: obj=0x7fe3bc185f50
2018-02-01 03:21:20.803+0000: 12114: debug : qemuProcessKill:5967 : vm=0x7fe3bc185f50 name=generic pid=12700 flags=5
2018-02-01 03:21:20.803+0000: 12114: debug : virProcessKillPainfully:355 : vpid=12700 force=1
2018-02-01 03:21:20.803+0000: 12114: debug : qemuDomainCleanupRun:5443 : driver=0x7fe3bc1bc1e0, vm=generic
2018-02-01 03:21:20.803+0000: 12114: debug : qemuProcessAutoDestroyRemove:6674 : vm=generic
2018-02-01 03:21:20.803+0000: 12114: debug : virCloseCallbacksUnset:162 : vm=generic, uuid=d4ae2244-1465-4fb2-a3b1-3c2581172582, cb=0x7fe3d712fa00
2018-02-01 03:21:20.803+0000: 12114: debug : virCommandRunAsync:2459 : About to run /usr/lib/libvirt/virt-aa-helper -R -u libvirt-d4ae2244-1465-4fb2-a3b1-3c2581172582
2018-02-01 03:21:20.804+0000: 12114: debug : virFileClose:110 : Closed fd 17
2018-02-01 03:21:20.804+0000: 12114: debug : virFileClose:110 : Closed fd 26
2018-02-01 03:21:20.804+0000: 12114: debug : virFileClose:110 : Closed fd 28
2018-02-01 03:21:20.804+0000: 12114: debug : virCommandRunAsync:2462 : Command result 0, with PID 12840
2018-02-01 03:21:20.805+0000: 12840: debug : virFileClose:110 : Closed fd 3
2018-02-01 03:21:20.805+0000: 12840: debug : virFileClose:110 : Closed fd 4
2018-02-01 03:21:20.805+0000: 12840: debug : virFileClose:110 : Closed fd 5
2018-02-01 03:21:20.805+0000: 12840: debug : virFileClose:110 : Closed fd 6
2018-02-01 03:21:20.805+0000: 12840: debug : virFileClose:110 : Closed fd 7
2018-02-01 03:21:20.805+0000: 12840: debug : virFileClose:110 : Closed fd 8
2018-02-01 03:21:20.805+0000: 12840: debug : virFileClose:110 : Closed fd 9
2018-02-01 03:21:20.806+0000: 12840: debug : virFileClose:110 : Closed fd 10
2018-02-01 03:21:20.806+0000: 12840: debug : virFileClose:110 : Closed fd 11
2018-02-01 03:21:20.806+0000: 12840: debug : virFileClose:110 : Closed fd 12
2018-02-01 03:21:20.806+0000: 12840: debug : virFileClose:110 : Closed fd 13
2018-02-01 03:21:20.806+0000: 12840: debug : virFileClose:110 : Closed fd 14
2018-02-01 03:21:20.806+0000: 12840: debug : virFileClose:110 : Closed fd 15
2018-02-01 03:21:20.806+0000: 12840: debug : virFileClose:110 : Closed fd 16
2018-02-01 03:21:20.806+0000: 12840: debug : virFileClose:110 : Closed fd 18
2018-02-01 03:21:20.806+0000: 12840: debug : virFileClose:110 : Closed fd 19
2018-02-01 03:21:20.806+0000: 12840: debug : virFileClose:110 : Closed fd 20
2018-02-01 03:21:20.806+0000: 12840: debug : virFileClose:110 : Closed fd 21
2018-02-01 03:21:20.806+0000: 12840: debug : virFileClose:110 : Closed fd 22
2018-02-01 03:21:20.806+0000: 12840: debug : virFileClose:110 : Closed fd 23
2018-02-01 03:21:20.806+0000: 12840: debug : virFileClose:110 : Closed fd 24
2018-02-01 03:21:20.806+0000: 12840: debug : virFileClose:110 : Closed fd 25
2018-02-01 03:21:20.806+0000: 12840: debug : virFileClose:110 : Closed fd 27
2018-02-01 03:21:20.931+0000: 12114: debug : virCommandRun:2310 : Result status 0, stdout: '' stderr: '2018-02-01 03:21:20.806+0000: 12840: debug : virFileClose:110 : Closed fd 26
2018-02-01 03:21:20.806+0000: 12840: debug : virFileClose:110 : Closed fd 28
2018-02-01 03:21:20.806+0000: 12840: debug : virFileClose:110 : Closed fd 17
'
2018-02-01 03:21:20.931+0000: 12114: debug : virFileClose:110 : Closed fd 23
2018-02-01 03:21:20.931+0000: 12114: debug : virFileClose:110 : Closed fd 27
2018-02-01 03:21:20.931+0000: 12114: debug : virSecurityDACRestoreAllLabel:1474 : Restoring security label on generic migrated=0
2018-02-01 03:21:20.931+0000: 12114: info : virSecurityDACRestoreFileLabelInternal:654 : Restoring DAC user and group on '/var/lib/libvirt/images/generic.qcow2'
2018-02-01 03:21:20.931+0000: 12114: info : virSecurityDACSetOwnershipInternal:556 : Setting DAC user and group on '/var/lib/libvirt/images/generic.qcow2' to '0:0'
2018-02-01 03:21:20.931+0000: 12114: info : virObjectRef:365 : OBJECT_REF: obj=0x7fe3bc00e860
2018-02-01 03:21:20.931+0000: 12114: debug : networkReleaseActualDevice:5021 : Nothing to release to network default
2018-02-01 03:21:20.931+0000: 12114: debug : virJSONValueToString:1881 : object=0x7fe3bc2ba770
2018-02-01 03:21:20.931+0000: 12114: debug : virJSONValueToStringOne:1810 : object=0x7fe3bc2ba770 type=1 gen=0x7fe3bc257d70
2018-02-01 03:21:20.931+0000: 12114: debug : virJSONValueToString:1914 : result=[

]

2018-02-01 03:21:20.938+0000: 12114: debug : virFileClose:110 : Closed fd 17
2018-02-01 03:21:20.938+0000: 12114: info : networkLogAllocation:4307 : MAC 52:54:00:75:8e:77 releasing network default (0 connections)
2018-02-01 03:21:20.939+0000: 12114: info : virObjectUnref:327 : OBJECT_UNREF: obj=0x7fe3bc00e860
2018-02-01 03:21:20.939+0000: 12114: debug : qemuRemoveCgroup:1147 : Failed to terminate cgroup for generic
2018-02-01 03:21:20.939+0000: 12114: debug : virCgroupRemove:3513 : Removing cgroup /machine/qemu-8-generic.libvirt-qemu
2018-02-01 03:21:20.939+0000: 12114: debug : virCgroupRemove:3534 : Removing cgroup /sys/fs/cgroup/cpu,cpuacct/machine/qemu-8-generic.libvirt-qemu/ and all child cgroups
2018-02-01 03:21:20.939+0000: 12114: debug : virCgroupRemoveRecursively:3484 : Removing cgroup /sys/fs/cgroup/cpu,cpuacct/machine/qemu-8-generic.libvirt-qemu//vcpu0
2018-02-01 03:21:20.939+0000: 12114: debug : virCgroupRemoveRecursively:3484 : Removing cgroup /sys/fs/cgroup/cpu,cpuacct/machine/qemu-8-generic.libvirt-qemu//emulator
2018-02-01 03:21:20.939+0000: 12114: debug : virCgroupRemoveRecursively:3484 : Removing cgroup /sys/fs/cgroup/cpu,cpuacct/machine/qemu-8-generic.libvirt-qemu/
2018-02-01 03:21:20.939+0000: 12114: debug : virCgroupRemove:3534 : Removing cgroup /sys/fs/cgroup/cpu,cpuacct/machine/qemu-8-generic.libvirt-qemu/ and all child cgroups
2018-02-01 03:21:20.939+0000: 12114: debug : virCgroupRemove:3534 : Removing cgroup /sys/fs/cgroup/cpuset/machine/qemu-8-generic.libvirt-qemu/ and all child cgroups
2018-02-01 03:21:20.939+0000: 12114: debug : virCgroupRemoveRecursively:3484 : Removing cgroup /sys/fs/cgroup/cpuset/machine/qemu-8-generic.libvirt-qemu//vcpu0
2018-02-01 03:21:20.939+0000: 12114: debug : virCgroupRemoveRecursively:3484 : Removing cgroup /sys/fs/cgroup/cpuset/machine/qemu-8-generic.libvirt-qemu//emulator
2018-02-01 03:21:20.939+0000: 12114: debug : virCgroupRemoveRecursively:3484 : Removing cgroup /sys/fs/cgroup/cpuset/machine/qemu-8-generic.libvirt-qemu/
2018-02-01 03:21:20.939+0000: 12114: debug : virCgroupRemove:3534 : Removing cgroup /sys/fs/cgroup/memory/machine/qemu-8-generic.libvirt-qemu/ and all child cgroups
2018-02-01 03:21:20.939+0000: 12114: debug : virCgroupRemoveRecursively:3484 : Removing cgroup /sys/fs/cgroup/memory/machine/qemu-8-generic.libvirt-qemu/
2018-02-01 03:21:20.939+0000: 12114: debug : virCgroupRemove:3534 : Removing cgroup /sys/fs/cgroup/devices/machine/qemu-8-generic.libvirt-qemu/ and all child cgroups
2018-02-01 03:21:20.939+0000: 12114: debug : virCgroupRemoveRecursively:3484 : Removing cgroup /sys/fs/cgroup/devices/machine/qemu-8-generic.libvirt-qemu/
2018-02-01 03:21:20.939+0000: 12114: debug : virCgroupRemove:3534 : Removing cgroup /sys/fs/cgroup/freezer/machine/qemu-8-generic.libvirt-qemu/ and all child cgroups
2018-02-01 03:21:20.939+0000: 12114: debug : virCgroupRemoveRecursively:3484 : Removing cgroup /sys/fs/cgroup/freezer/machine/qemu-8-generic.libvirt-qemu/
2018-02-01 03:21:20.939+0000: 12114: debug : virCgroupRemove:3534 : Removing cgroup /sys/fs/cgroup/blkio/machine/qemu-8-generic.libvirt-qemu/ and all child cgroups
2018-02-01 03:21:20.939+0000: 12114: debug : virCgroupRemoveRecursively:3484 : Removing cgroup /sys/fs/cgroup/blkio/machine/qemu-8-generic.libvirt-qemu/
2018-02-01 03:21:20.939+0000: 12114: debug : virCgroupRemove:3534 : Removing cgroup /sys/fs/cgroup/net_cls,net_prio/machine/qemu-8-generic.libvirt-qemu/ and all child cgroups
2018-02-01 03:21:20.939+0000: 12114: debug : virCgroupRemoveRecursively:3484 : Removing cgroup /sys/fs/cgroup/net_cls,net_prio/machine/qemu-8-generic.libvirt-qemu/
2018-02-01 03:21:20.939+0000: 12114: debug : virCgroupRemove:3534 : Removing cgroup /sys/fs/cgroup/perf_event/machine/qemu-8-generic.libvirt-qemu/ and all child cgroups
2018-02-01 03:21:20.939+0000: 12114: debug : virCgroupRemoveRecursively:3484 : Removing cgroup /sys/fs/cgroup/perf_event/machine/qemu-8-generic.libvirt-qemu/
2018-02-01 03:21:20.939+0000: 12114: debug : virCgroupRemove:3538 : Done removing cgroup /machine/qemu-8-generic.libvirt-qemu
2018-02-01 03:21:20.939+0000: 12114: info : virObjectRef:365 : OBJECT_REF: obj=0x7fe3bc24e5a0
2018-02-01 03:21:20.939+0000: 12114: info : virObjectUnref:327 : OBJECT_UNREF: obj=0x7fe3bc24e5a0
2018-02-01 03:21:20.939+0000: 12114: info : virObjectUnref:327 : OBJECT_UNREF: obj=0x7fe408000d80
2018-02-01 03:21:20.939+0000: 12114: info : virObjectUnref:329 : OBJECT_DISPOSE: obj=0x7fe408000d80
2018-02-01 03:21:20.939+0000: 12114: info : virObjectUnref:327 : OBJECT_UNREF: obj=0x7fe408000a80
2018-02-01 03:21:20.939+0000: 12114: info : virObjectUnref:329 : OBJECT_DISPOSE: obj=0x7fe408000a80
2018-02-01 03:21:20.939+0000: 12114: info : virObjectUnref:327 : OBJECT_UNREF: obj=0x7fe408001310
2018-02-01 03:21:20.939+0000: 12114: info : virObjectUnref:329 : OBJECT_DISPOSE: obj=0x7fe408001310
2018-02-01 03:21:20.939+0000: 12114: info : virObjectUnref:327 : OBJECT_UNREF: obj=0x7fe404004f70
2018-02-01 03:21:20.939+0000: 12114: info : virObjectUnref:329 : OBJECT_DISPOSE: obj=0x7fe404004f70
2018-02-01 03:21:20.939+0000: 12114: info : virObjectUnref:327 : OBJECT_UNREF: obj=0x7fe404004fd0
2018-02-01 03:21:20.939+0000: 12114: info : virObjectUnref:329 : OBJECT_DISPOSE: obj=0x7fe404004fd0
2018-02-01 03:21:20.939+0000: 12114: info : virObjectUnref:327 : OBJECT_UNREF: obj=0x7fe404018280
2018-02-01 03:21:20.939+0000: 12114: info : virObjectUnref:329 : OBJECT_DISPOSE: obj=0x7fe404018280
2018-02-01 03:21:20.939+0000: 12114: info : virObjectUnref:327 : OBJECT_UNREF: obj=0x7fe4040183b0
2018-02-01 03:21:20.940+0000: 12114: info : virObjectUnref:329 : OBJECT_DISPOSE: obj=0x7fe4040183b0
2018-02-01 03:21:20.940+0000: 12114: info : virObjectUnref:327 : OBJECT_UNREF: obj=0x7fe4040181c0
2018-02-01 03:21:20.940+0000: 12114: info : virObjectUnref:329 : OBJECT_DISPOSE: obj=0x7fe4040181c0
2018-02-01 03:21:20.940+0000: 12114: info : virObjectUnref:327 : OBJECT_UNREF: obj=0x7fe404017e90
2018-02-01 03:21:20.940+0000: 12114: info : virObjectUnref:329 : OBJECT_DISPOSE: obj=0x7fe404017e90
2018-02-01 03:21:20.940+0000: 12114: info : virObjectUnref:327 : OBJECT_UNREF: obj=0x7fe404019350
2018-02-01 03:21:20.940+0000: 12114: info : virObjectUnref:329 : OBJECT_DISPOSE: obj=0x7fe404019350
2018-02-01 03:21:20.940+0000: 12114: info : virObjectUnref:327 : OBJECT_UNREF: obj=0x7fe404019650
2018-02-01 03:21:20.940+0000: 12114: info : virObjectUnref:329 : OBJECT_DISPOSE: obj=0x7fe404019650
2018-02-01 03:21:20.940+0000: 12114: info : virObjectUnref:327 : OBJECT_UNREF: obj=0x7fe3bc24e5a0
2018-02-01 03:21:20.940+0000: 12114: info : virObjectUnref:327 : OBJECT_UNREF: obj=0x7fe3bc26b900
2018-02-01 03:21:20.940+0000: 12114: info : virObjectUnref:329 : OBJECT_DISPOSE: obj=0x7fe3bc26b900
2018-02-01 03:21:20.940+0000: 12114: debug : virDomainEventLifecycleDispose:442 : obj=0x7fe3bc26b900
2018-02-01 03:21:20.940+0000: 12114: debug : virDomainEventDispose:435 : obj=0x7fe3bc26b900
2018-02-01 03:21:20.940+0000: 12114: debug : virObjectEventDispose:134 : obj=0x7fe3bc26b900
2018-02-01 03:21:20.940+0000: 12114: debug : qemuDomainObjEndJob:4046 : Stopping job: destroy (async=none vm=0x7fe3bc185f50 name=generic)
2018-02-01 03:21:20.940+0000: 12114: info : virObjectRef:365 : OBJECT_REF: obj=0x7fe3bc24e5a0
2018-02-01 03:21:20.940+0000: 12114: info : virObjectUnref:327 : OBJECT_UNREF: obj=0x7fe3bc24e5a0
2018-02-01 03:21:20.940+0000: 12114: info : virObjectUnref:327 : OBJECT_UNREF: obj=0x7fe3bc185f50
2018-02-01 03:21:24.932+0000: 11815: debug : virEventPollRunOnce:650 : Poll got 0 event(s)
2018-02-01 03:21:24.932+0000: 11815: debug : virEventPollDispatchTimeouts:432 : Dispatch 2
2018-02-01 03:21:24.932+0000: 11815: info : virEventPollDispatchTimeouts:455 : EVENT_POLL_DISPATCH_TIMEOUT: timer=2
2018-02-01 03:21:24.932+0000: 11815: info : virObjectRef:365 : OBJECT_REF: obj=0x55d686a8efe0
2018-02-01 03:21:24.932+0000: 11815: info : virKeepAliveTimerInternal:136 : RPC_KEEPALIVE_TIMEOUT: ka=0x55d686a8efe0 client=0x55d686a8e6c0 countToDeath=5 idle=5
2018-02-01 03:21:24.932+0000: 11815: debug : virNetMessageNew:46 : msg=0x55d686a8dad0 tracked=0
2018-02-01 03:21:24.932+0000: 11815: debug : virNetMessageEncodePayloadEmpty:492 : Encode length as 28
2018-02-01 03:21:24.932+0000: 11815: debug : virKeepAliveMessage:104 : Sending keepalive request to client 0x55d686a8e6c0
2018-02-01 03:21:24.932+0000: 11815: info : virKeepAliveMessage:107 : RPC_KEEPALIVE_SEND: ka=0x55d686a8efe0 client=0x55d686a8e6c0 prog=1801807216 vers=1 proc=1
2018-02-01 03:21:24.932+0000: 11815: info : virEventPollUpdateTimeout:265 : EVENT_POLL_UPDATE_TIMEOUT: timer=2 frequency=5000
2018-02-01 03:21:24.932+0000: 11815: debug : virEventPollUpdateTimeout:282 : Set timer freq=5000 expires=1517455289932
2018-02-01 03:21:24.932+0000: 11815: debug : virEventPollInterruptLocked:722 : Skip interrupt, 1 140617919043904
2018-02-01 03:21:24.932+0000: 11815: debug : virNetServerClientSendMessageLocked:1480 : msg=0x55d686a8dad0 proc=1 len=28 offset=0
2018-02-01 03:21:24.932+0000: 11815: info : virNetServerClientSendMessageLocked:1488 : RPC_SERVER_CLIENT_MSG_TX_QUEUE: client=0x55d686a8e6c0 len=28 prog=1801807216 vers=1 proc=1 type=2 status=0 serial=0
2018-02-01 03:21:24.932+0000: 11815: debug : virNetServerClientCalculateHandleMode:166 : tls=(nil) hs=-1, rx=0x55d686a8f710 tx=0x55d686a8dad0
2018-02-01 03:21:24.932+0000: 11815: debug : virNetServerClientCalculateHandleMode:201 : mode=3
2018-02-01 03:21:24.932+0000: 11815: info : virEventPollUpdateHandle:152 : EVENT_POLL_UPDATE_HANDLE: watch=13 events=3
2018-02-01 03:21:24.932+0000: 11815: debug : virEventPollInterruptLocked:722 : Skip interrupt, 1 140617919043904
2018-02-01 03:21:24.932+0000: 11815: info : virObjectUnref:327 : OBJECT_UNREF: obj=0x55d686a8efe0
2018-02-01 03:21:24.932+0000: 11815: debug : virEventPollCleanupTimeouts:525 : Cleanup 2
2018-02-01 03:21:24.932+0000: 11815: debug : virEventPollCleanupHandles:574 : Cleanup 12
2018-02-01 03:21:24.932+0000: 11815: debug : virEventRunDefaultImpl:311 : running default event implementation
2018-02-01 03:21:24.932+0000: 11815: debug : virEventPollCleanupTimeouts:525 : Cleanup 2
2018-02-01 03:21:24.932+0000: 11815: debug : virEventPollCleanupHandles:574 : Cleanup 12
2018-02-01 03:21:24.932+0000: 11815: debug : virEventPollMakePollFDs:401 : Prepare n=0 w=1, f=4 e=1 d=0
2018-02-01 03:21:24.932+0000: 11815: debug : virEventPollMakePollFDs:401 : Prepare n=1 w=2, f=6 e=1 d=0
2018-02-01 03:21:24.932+0000: 11815: debug : virEventPollMakePollFDs:401 : Prepare n=2 w=3, f=9 e=1 d=0
2018-02-01 03:21:24.932+0000: 11815: debug : virEventPollMakePollFDs:401 : Prepare n=3 w=4, f=10 e=1 d=0
2018-02-01 03:21:24.932+0000: 11815: debug : virEventPollMakePollFDs:401 : Prepare n=4 w=5, f=11 e=1 d=0
2018-02-01 03:21:24.932+0000: 11815: debug : virEventPollMakePollFDs:401 : Prepare n=5 w=6, f=12 e=1 d=0
2018-02-01 03:21:24.932+0000: 11815: debug : virEventPollMakePollFDs:401 : Prepare n=6 w=7, f=14 e=1 d=0
2018-02-01 03:21:24.932+0000: 11815: debug : virEventPollMakePollFDs:401 : Prepare n=7 w=8, f=13 e=0 d=0
2018-02-01 03:21:24.932+0000: 11815: debug : virEventPollMakePollFDs:401 : Prepare n=8 w=9, f=13 e=1 d=0
2018-02-01 03:21:24.932+0000: 11815: debug : virEventPollMakePollFDs:401 : Prepare n=9 w=10, f=19 e=1 d=0
2018-02-01 03:21:24.932+0000: 11815: debug : virEventPollMakePollFDs:401 : Prepare n=10 w=12, f=24 e=1 d=0
