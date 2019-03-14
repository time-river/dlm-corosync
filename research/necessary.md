# 必要性

数据损坏(Data corruption)是一种现象，指的是在对计算机中存储的数据执行读、写、存储、传输或者处理的过程中，导致原始数据产生了未曾预想的变化，数据变得不再可靠。

为了保证数据的可靠性，QEMU在2.9与2.10两个版本中分别引入了两项特性：

- `share-rw`qdev(QEMU device)属性(Patch: dabd18f64c8800d441fd9fb232c2102e8409aa2e)- drive的`file.locking`选项(Patch: 16b48d5d66d2fceb86114b28d693aad930f5070d)


Note:

    QCOW2不支持`share-rw=on`，raw与block设备支持
