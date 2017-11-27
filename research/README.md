libvirt实现了一个框架，用于确保虚拟机在共享宿主机资源的时候能够实现资源的互斥访问。典型的应用场景是实现一个写锁，阻止两个虚拟机进程同时获取共享磁盘镜像的写权限，因为不这样的话，在不使用集群文件系统的客户机上会导致数据损坏。

#### 目标
阻止多个QEMU实例同时使用同一个磁盘镜像，除非这个镜像被标识为共享的，或者只读。要实现资源互斥的场景是：
1. 两个不同的客户机运行在同一个磁盘上
2. 因为管理员失误导致的一个客户机在不同的宿主机上多次启动
3. 因此libvirt的驱动bug导致的一个客户机在同一个宿主机上多次启动

#### 要求
为了实现所设定的目标，锁管理器提出了以下设计要求：
1. QEMU进程打开磁盘的时候，这个磁盘必须被加锁
2. 必须实现只读、共享写入、互斥写三种锁的模式
3. 在QEMU实例迁移的时候，对共享磁盘镜像锁的切换必须可行
4. 如果某个资源的锁被移除，锁管理器必须能够识别并杀死QEMU实例对该资源的访问(可逆)
5. 通过管理程序可以对任意的虚拟机资源进行加锁

#### 设计
以下一系列操作需要被锁管理器支持：
1. __Register object__ 注册一个对象，该对象的标识符不能重复，用于锁的获取
2. __Add resource__ 把对象与资源相关联，用于未来的互斥操作
3. __Acquire locks__ 对与对象所关联的所有资源进行加锁
4. __Release locks__ 释放与对象所关联的所有资源的锁
5. __Inquire locks__ 查询与该对象所关联的锁的状态

#### 插件的实现
锁管理器源码的许可证书是LGPLv2+，使用`dlopen()`模块。插件将会从下面的地址进行加载：
```
/usr/{lib,lib64}/libvirt/lock_manager/$NAME.so
```
在锁管理器插件中必须存在一个名为`virLockDriverImpl`的ELF符号，它是一个`struct virLockDriver`类型的全局变量，这个结构体在下面的头文件中定义
```
#include <libvirt/plugins/lock_manager.h>
```
结构体成员中的回调函数指针必须被初始化为非`NULL`，相应的函数指针的原型也在上述头文件中定义。

##### 实现细节

qemuStateInitialize // 确定加载哪个插件
 -- virLockManagerPluginNew
     -- if (STREQ(name, "nop")) {
            driver = &virLockDriverNop
        } else {
            dlopen // 打开动态链接库
            dlsym  // 返回动态链接库中virLockDriverImpl符号的偏移量
        }
        driver->drvInit // 初始化锁管理器
        plugin->driver = driver;
        plugin->handle = handle;
        plugin->refs = 1;
        VIR_STRDUP(plugin->name, name)

```
struct _virLockManagerPlugin {      
    char *name;              // nop | lockd | sanlock
    virLockDriverPtr driver; // virLockDriverImpl
    void *handle;            // dlopen 的返回值
    int refs;                // 1
};

struct _virLockManagerPlugin plugin;
```

#### QEMU 驱动集成
若要当作QEMU的驱动程序使用，锁插件必须在`/etc/libvirt/qemu.conf`配置文件中配置锁管理器的名称：
```
lockManager="sanlock"
```
默认的锁管理器是'no op'实现，为的是向后兼容。

#### 锁的使用模式
以下伪代码说明了在锁管理器插件中通用的回调函数内容。

##### Lock acquisition
初始化时候锁的获取会从拥有这把锁的进程中执行。比较典型的是QEMU的子进程，由`fork+exec`生成。在动态添加更多资源时，为了保持锁的现有对象，这些工作将在libvirt的进程中完成。

```
virLockManagerParam params[] = {
  { .type = VIR_LOCK_MANAGER_PARAM_TYPE_UUID,
    .key = "uuid",
  },
  { .type = VIR_LOCK_MANAGER_PARAM_TYPE_STRING,
    .key = "name",
    .value = { .str = dom->def->name },
  },
  { .type = VIR_LOCK_MANAGER_PARAM_TYPE_UINT,
    .key = "id",
    .value = { .i = dom->def->id },
  },
  { .type = VIR_LOCK_MANAGER_PARAM_TYPE_UINT,
    .key = "pid",
    .value = { .i = dom->pid },
  },
  { .type = VIR_LOCK_MANAGER_PARAM_TYPE_CSTRING,
    .key = "uri",
    .value = { .cstr = driver->uri },
  },
};
mgr = virLockManagerNew(lockPlugin,
                        VIR_LOCK_MANAGER_TYPE_DOMAIN,
                        ARRAY_CARDINALITY(params),
                        params,
                        0)));

foreach (initial disks)
    virLockManagerAddResource(mgr,
                              VIR_LOCK_MANAGER_RESOURCE_TYPE_DISK,
                              $path, 0, NULL, $flags);

if (virLockManagerAcquire(lock, NULL, 0) < 0);
  ...abort...
```

##### Lock release
当拥有锁的进程退出的时候，锁都可以隐式地释放，但进程也可以运行下面的代码主动释放
```
char *state = NULL;
virLockManagerParam params[] = {
  { .type = VIR_LOCK_MANAGER_PARAM_TYPE_UUID,
    .key = "uuid",
  },
  { .type = VIR_LOCK_MANAGER_PARAM_TYPE_STRING,
    .key = "name",
    .value = { .str = dom->def->name },
  },
  { .type = VIR_LOCK_MANAGER_PARAM_TYPE_UINT,
    .key = "id",
    .value = { .i = dom->def->id },
  },
  { .type = VIR_LOCK_MANAGER_PARAM_TYPE_UINT,
    .key = "pid",
    .value = { .i = dom->pid },
  },
  { .type = VIR_LOCK_MANAGER_PARAM_TYPE_CSTRING,
    .key = "uri",
    .value = { .cstr = driver->uri },
  },
};
mgr = virLockManagerNew(lockPlugin,
                        VIR_LOCK_MANAGER_TYPE_DOMAIN,
                        ARRAY_CARDINALITY(params),
                        params,
                        0)));

foreach (initial disks)
    virLockManagerAddResource(mgr,
                              VIR_LOCK_MANAGER_RESOURCE_TYPE_DISK,
                              $path, 0, NULL, $flags);

virLockManagerRelease(mgr, & state, 0);
```
返回的状态字符串可以传递给`virLockManagerAcquire`方法，以便后续重新获取完全相同的锁。执行虚拟机热迁移的时候就是这种情况。通过验证锁管理器的状态可以确保在其他宿主机上，没有虚拟机重新获取同一把锁。通过调用`virLockManagerInquire`方法也可以在不释放锁的情况下获得锁的状态。
