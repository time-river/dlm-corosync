
这是我大学的毕业设计——《Libvirt 分布式锁管理插件DLM-Corosync的实现》的相关内容。

确定毕业设计题目的时候，是在17年10月下旬，当时我还在SUSE实习。原因种种，令我决定
在SUSE做我的毕业设计。题目来源于我当时的leader [Roger](https://www.linkedin.com/in/roger-zhou-%E5%91%A8%E5%BF%97%E5%BC%BA-63446914/)。

# 背景

Libvirt中的锁管理器（lock manager）框架的实现使得虚拟机在共享宿主机资源的时候能
够实现对资源的互斥访问。典型的场景是实现一个写锁，阻止两个虚拟机进程同时获取共享
磁盘镜像的写入权限。这一技术的实现不仅可以保证多个KVM/QEMU实例不同时使用一个磁盘
镜像，同时也能够显式地标识所共享的磁盘。

Libvirt在锁管理器的框架下实现了nop、sanlock与lockd三种锁。Nop代表着什么都不做。
Sanlock使用disk-paxos算法读写共享存储以实现对分布式锁的获取、释放和超时。但
sanlock 的使用会带来大量的磁盘I/O操作，因此又提出了lockd这种使用标准POSIX文件锁
的实现。

但这种类型的数据损坏仍然在发生——[Bug 1378241 - QEMU image file locking](https://bugzilla.redhat.com/show_bug.cgi?id=1378241)，这是因为有些独立于libvirt之外工具（比
如qemu-img）的使用并不能检测到磁盘镜像是否被别的进程使用。Libvirt社区中有成员期
望着使用其他机制的锁管理器插件的出现。

# 介绍

DLM全称Distributed Lock Manager，是一种建议性锁，用以协调共享资源的使用，从而
允许在集群中的多个节点上并发地运行程序。协作的程序可以运行在集群的不同节点上，
共享一个资源而不会导致它损坏。Linux 内核实现了DLM，并在诸如 OCFS2 等的集群文件
系统中获得了广泛的使用。

Corosync 全称 The Corosync Cluster Engine，它是集群节点之间通信的基础设施。

虽然无法解决数据损坏的问题，但因为DLM-Corosync在一些高可用套件中被广泛使用，因此
在一些场景的使用上更加方便。

[[libvirt] RFC: Introduce a dlm-corosync for Lock manager plugin](https://www.redhat.com/archives/libvir-list/2017-December/msg00689.html)


## 仓库目录结构

```
├── design -- 设计上的考虑
├── LICENSE
├── patches
│   ├── v1 -- 第一版本的patch set
│   └── v2 -- 第二版本的patch set
├── README.md
├── research -- 调研时看到的资料
│   └── experiment -- demo代码
└── src
    ├── v1 -- 第一版本的核心代码
    └── v2 -- 第二版本的核心代码
```

# 结果

最终的实现patch set在[这里](patches/v2)。src目录下是相关的源码。

完成的第一个版本的patch set在[这里](patches/v1)。
这里是往社区的提交记录：[[libvirt] [PATCH 0/3] lock manager plugin dlm-corosync	implementation](https://www.redhat.com/archives/libvir-list/2018-February/msg00185.html)

完成第二个版本后，仔细阅读QEMU新的feature[[Qemu-devel] [PATCH for-2.7 00/15] block: Lock images when openin](https://lists.gnu.org/archive/html/qemu-devel/2016-04/msg02100.html)之后，
自认认为DLM-Corosync plugin价值不大，这是因为：QEMU使用
[Open File Description lock](https://lwn.net/Articles/586904/)后，功能与libvirt的
lock manager重复。虽然libvirt不仅仅可以管理QEMU/KVM，但它毕竟是主流。

因此放弃提交给社区，项目结束。
