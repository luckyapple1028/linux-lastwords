Linux内核临终遗言

内核版本：Linux-4.1.12
调试单板：树莓派1b
功能：跟踪检测内核模块加载变化、网络设备变化、D状态和R状态死锁、reboot、oom以及Panic等，将信息记录到保留的内存空间中，可用于系统恢复后查看内核异常记录信息。

文件介绍：
1、内核模块
（1）lastwords_main：临终遗言模块入口程序，负责初始化和去初始化临终遗言子系统；
（2）lastwords_mem：保留内存操作程序，负责映射用于记录遗言信息的保留内存；
（3）lastwords_record：日志信息记录程序，负责记录和读取保留内存日志信息；
（4）lastwords_export: 日志格式输出程序，负责按照指定的格式写日志信息；
（5）lastwords_interface：用户接口程序，负责同应用程序交互；
（6）lastwords_monitor：内核状态监视程序，负责监视Linux系统内核及进程状态。

2、应用程序
（1）lastwords_user：用户层控制和解析输出程序。






