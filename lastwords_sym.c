#include <linux/module.h>  
#include <linux/kernel.h>  
#include <linux/init.h>  
#include <linux/types.h>
#include <linux/sched.h>
#include <linux/kallsyms.h>

#include "lastwords.h"
#include "lastwords_sym.h"

/* 函数变量查找结构 */
struct lws_sym {
	void **addr;		/* 参数地址 */
	char *name;			/* 参数名 */
};

/* 内核变量定义 */
int *lw_nr_threads = NULL;									/* nr_threads */	


/* 内核函数定义 */
lw__current_kernel_time_fun lw__current_kernel_time;		/* __current_kernel_time */




/* 函数变量查找结构 */
struct lws_sym lws_sym_val[] = {
	/* 变量 */
	{(void**)&lw_nr_threads, "nr_threads"},
	/* 函数 */
	{(void**)&lw__current_kernel_time, "__current_kernel_time"}
};


/* ******************************************
 * 函 数 名: lastwords_init_sym
 * 功能描述: 临终遗言初始化函数变量地址
 * 输入参数: void
 * 输出参数: void	
 * 返 回 值: 成功返回0，失败返回错误码
 * ****************************************** */
int lastwords_init_sym(void)
{
	unsigned long addr = 0;
	int i = 0;
	
	for (i = 0; i < sizeof(lws_sym_val)/sizeof(struct lws_sym); i++) {
		addr = kallsyms_lookup_name(lws_sym_val[i].name);
		if (0 == addr) {
			pr_err("Lastwords cannot find sym %s\n", lws_sym_val[i].name);
			return -1;
		}
		
		*(lws_sym_val[i].addr) = (void *)addr;
	}
		
	return 0;
}

/* ******************************************
 * 函 数 名: lastwords_exit_sym
 * 功能描述: 临终遗言初去始化函数变量地址
 * 输入参数: void
 * 输出参数: void	
 * 返 回 值: void
 * ****************************************** */
void lastwords_exit_sym(void)
{
	/* to do */
}


