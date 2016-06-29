#include <linux/module.h>  
#include <linux/kernel.h>  
#include <linux/init.h>  
#include <linux/notifier.h>
#include <linux/netdevice.h>
#include <linux/reboot.h>
#include <linux/net.h>
#include <linux/kdebug.h>
#include <linux/oom.h>
#include <net/netevent.h>

#include "lastwords.h"
#include "lastwords_record.h"
#include "lastwords_export.h"


/* oom输出类型 */										
									

/* die输出类型 */


/* reboot输出类型 */
#define LASTWORDS_REBOOT_EXPORT			( LASTWORDS_EXPORT_TIME \
										| LASTWORDS_EXPORT_SYSINFO \
										| LASTWORDS_EXPORT_DMESG \
										| LASTWORDS_EXPORT_PS )





/* ******************************************
 * 函 数 名: lastwords_oom_callback
 * 功能描述: 临终遗言oom内核通知链回调
 * 输入参数: struct notifier_block *this: 通知链结构
 			 unsigned long event: 触发类型
 			 void *ptr: 触发参数
 * 输出参数: void	
 * 返 回 值: 返回NOTIFY_DONE
 * ****************************************** */
static int lastwords_oom_callback(struct notifier_block *this, 
				unsigned long event, void *ptr)
{
	return NOTIFY_DONE;
}

static struct notifier_block lastwords_oom_notifier = {
	.notifier_call = lastwords_oom_callback,
};


/* ******************************************
 * 函 数 名: lastwords_die_callback
 * 功能描述: 临终遗言die内核通知链回调
 * 输入参数: struct notifier_block *this: 通知链结构
 			 unsigned long event: 触发类型
 			 void *ptr: 触发参数
 * 输出参数: void	
 * 返 回 值: 返回NOTIFY_DONE
 * ****************************************** */
static int lastwords_die_callback(struct notifier_block *this, 
				unsigned long event, void *ptr)
{
	return NOTIFY_DONE;
}

static struct notifier_block lastwords_die_notifier = {
	.notifier_call = lastwords_die_callback,
};


/* ******************************************
 * 函 数 名: lastwords_netevent_callback
 * 功能描述: 临终遗言网络状态变化内核通知链回调
 * 输入参数: struct notifier_block *this: 通知链结构
 			 unsigned long event: 触发类型
 			 void *ptr: 触发参数
 * 输出参数: void	
 * 返 回 值: 返回NOTIFY_DONE
 * ****************************************** */
static int lastwords_netevent_callback(struct notifier_block *this, 
				unsigned long event, void *ptr)
{
	return NOTIFY_DONE;
}

static struct notifier_block lastwords_netevent_notifier = {
	.notifier_call = lastwords_netevent_callback
};


/* ******************************************
 * 函 数 名: lastwords_netdev_event
 * 功能描述: 临终遗言网络设备内核通知链回调
 * 输入参数: struct notifier_block *this: 通知链结构
 			 unsigned long event: 触发类型
 			 void *ptr: 触发参数
 * 输出参数: void	
 * 返 回 值: 返回NOTIFY_DONE
 * ****************************************** */
static int lastwords_netdev_callback(struct notifier_block *this, 
				unsigned long event, void *ptr)
{
	struct net_device *dev = netdev_notifier_info_to_dev(ptr);
	struct net *net = dev_net(dev);

	switch (event) {
	/* 改变网络设备地址 */
	case NETDEV_CHANGEADDR:
		pr_info("net device change addr\n");
		
		break;
	/* 停用网络设备 */
	case NETDEV_DOWN:
		pr_info("net device down\n");
		
		break;
	/* 启用网络设备 */
	case NETDEV_UP:
		pr_info("net device up\n");

		break;
	default:
		break;
	}

	return NOTIFY_DONE;
}

static struct notifier_block lastwords_netdev_notifier = {
	.notifier_call = lastwords_netdev_callback,
};


/* ******************************************
 * 函 数 名: lastwords_reboot_callback
 * 功能描述: 临终遗言reboot内核通知链回调
 * 输入参数: struct notifier_block *this: 通知链结构
 			 unsigned long event: 触发类型
 			 void *ptr: 触发参数
 * 输出参数: void	
 * 返 回 值: 返回NOTIFY_DONE
 * ****************************************** */
static int lastwords_reboot_callback(struct notifier_block *this, 
				unsigned long event, void *ptr)
{
	(void)lastwords_export_attr(LAST_WORDS_ATTR_REBOOT, LASTWORDS_REBOOT_EXPORT, 
							"System reboot");

	return NOTIFY_DONE;
}

static struct notifier_block lastwords_reboot_notifier = {
	.notifier_call = lastwords_reboot_callback,
};


/* ******************************************
 * 函 数 名: lastwords_panic_callback
 * 功能描述: 临终遗panic内核通知链回调
 * 输入参数: struct notifier_block *this: 通知链结构
 			 unsigned long event: 触发类型
 			 void *ptr: 触发参数
 * 输出参数: void	
 * 返 回 值: 返回NOTIFY_DONE
 * ****************************************** */
static int lastwords_panic_callback(struct notifier_block *this, 
				unsigned long event, void *ptr)
{
	return NOTIFY_DONE;
}

static struct notifier_block lastwords_panic_notifier = { 
	.notifier_call = lastwords_panic_callback, 
};




/* ******************************************
 * 函 数 名: lastwords_init_monitor
 * 功能描述: 临终遗言监视器初始化
 * 输入参数: void
 * 输出参数: void	
 * 返 回 值: 成功返回0，失败返回错误码
 * ****************************************** */
int lastwords_init_monitor(void)
{
	int ret = 0;
	
	/* 注册网络设备通知链 */
	ret = register_netdevice_notifier(&lastwords_netdev_notifier);
	if (ret) {
		pr_err("Faile to register lastwords netdev notifier!\n");
		goto err_netdev;
	}

	/* 注册网络活动通知链 */
	ret = register_netevent_notifier(&lastwords_netevent_notifier);
	if (ret) {
		pr_err("Faile to register lastwords netevent notifier!\n");
		goto err_netevent;
	}
	
	/* 注册reboot通知链 */
	ret = register_reboot_notifier(&lastwords_reboot_notifier);
	if (ret) {
		pr_err("Faile to register reboot notifier!\n");
		goto err_reboot;
	}

	/* 注册die通知链 */
	ret = register_die_notifier(&lastwords_die_notifier);
	if (ret) {
		pr_err("Faile to register die notifier!\n");
		goto err_die;
	}

	/* 注册oom通知链 */
	ret = register_oom_notifier(&lastwords_oom_notifier);
	if (ret) {
		pr_err("Faile to register oom notifier!\n");
		goto err_oom;
	}	

	/* 注册panic通知链 */
	atomic_notifier_chain_register(&panic_notifier_list, &lastwords_panic_notifier);

	/* to do */
	
	return 0;

err_oom:
	unregister_die_notifier(&lastwords_die_notifier);
err_die:
	unregister_reboot_notifier(&lastwords_reboot_notifier);	
err_reboot:
	unregister_netevent_notifier(&lastwords_netevent_notifier);
err_netevent:
	unregister_netdevice_notifier(&lastwords_netdev_notifier);	
err_netdev:
	return ret;
}

/* ******************************************
 * 函 数 名: lastwords_exit_monitor
 * 功能描述: 临终遗言监视器去初始化
 * 输入参数: void
 * 输出参数: void	
 * 返 回 值: void
 * ****************************************** */
void lastwords_exit_monitor(void)
{
	/* 注销通知链 */
	atomic_notifier_chain_unregister(&panic_notifier_list, &lastwords_panic_notifier);	
	unregister_oom_notifier(&lastwords_oom_notifier);
	unregister_die_notifier(&lastwords_die_notifier);
	unregister_reboot_notifier(&lastwords_reboot_notifier);	
	unregister_netevent_notifier(&lastwords_netevent_notifier);	
	unregister_netdevice_notifier(&lastwords_netdev_notifier);
}


