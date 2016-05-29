Linux内核临终遗言

内核版本：Linux-4.1.12
调试单板：树莓派1b
功能：跟踪检测内核模块加载变化、网络设备变化、D状态和R状态死锁、reboot、oom以及Panic等，将信息记录到保留的内存空间中，可用于系统恢复后查看内核异常记录信息。

文件介绍：
（1）lastwords_main：临终遗言模块入口程序；
（2）lastwords_mem：保留内存操作程序；
（3）lastwords_record：日志信息记录程序；
（4）lastwords_interface：用户接口程序；
（5）lastwords_monitor：内核状态监视程序。








