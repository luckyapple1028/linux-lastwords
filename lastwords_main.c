#include <linux/module.h>  
#include <linux/kernel.h>  
#include <linux/init.h>  

#include "lastwords.h"

static int __init lastwords_init(void)
{
	int ret = 0;
	
	pr_notice("Linux Kernel Last Words Initing...\n");

	/* 1、初始化内存 */
	ret = lastwords_init_mem();
	if (0 != ret) {
		pr_err("Faile to init lastwords mem!\n");
		goto err_mem;
	}

	/* 2、初始化存储记录 */
	ret = lastwords_init_record();
	if (0 != ret) {
		pr_err("Faile to init lastwords record!\n");
		goto err_record;
	}

	/* 3、初始化用户接口 */
	ret = lastwords_init_interface();
	if (0 != ret) {
		pr_err("Faile to init lastwords user interface!\n");
		goto err_interface;		
	}

	/* 4、初始化监控 */
	/* to do */	
	pr_notice("Linux Kernel Last Words Init OK\n");
	return 0;
err_interface:
	lastwords_exit_record();
err_record:
	lastwords_exit_mem();	
err_mem:
	return ret;
}

static void __exit lastwords_exit(void) 
{
	lastwords_exit_interface();
	lastwords_exit_record();
	lastwords_exit_mem();
	
	pr_notice("Linux Kernel Last Words Exit\n");
}

module_init(lastwords_init);  
module_exit(lastwords_exit);  
MODULE_LICENSE("GPL");  

