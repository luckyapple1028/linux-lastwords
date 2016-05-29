#include <asm/io.h>

#include "lastwords.h"


/* 使用内存空间(0x1E000000~0x2000000)共32MB空间 */
#define LASTWORDS_MEM_ADDR		0x1E000000
#define LASTWORDS_MEM_SIZE		0x60//0x2000000			/* 32MB */

static phys_addr_t phyaddr = (phys_addr_t)LASTWORDS_MEM_ADDR;
static int memsize = LASTWORDS_MEM_SIZE;
static void __iomem *memaddr = NULL;



/* ******************************************
 * 函 数 名: lastwords_clean_mem
 * 功能描述: 临终遗言清空内存虚拟内存
 * 输入参数: void
 * 输出参数: void	
 * 返 回 值: void
 * ****************************************** */
void lastwords_clean_mem(void)
{
	if (memaddr)
		memset(memaddr, 0, memsize);
}

/* ******************************************
 * 函 数 名: lastwords_get_membase
 * 功能描述: 临终遗言获取内存虚拟基地址
 * 输入参数: void
 * 输出参数: void	
 * 返 回 值: 成功返回基地址，失败返回NULL
 * ****************************************** */
void * lastwords_get_membase(void)
{
	return memaddr;
}

/* ******************************************
 * 函 数 名: lastwords_get_memsize
 * 功能描述: 临终遗言获取内存长度
 * 输入参数: void
 * 输出参数: void	
 * 返 回 值: 返回内存长度
 * ****************************************** */
int lastwords_get_memsize(void)
{
	return memsize;
}

/* ******************************************
 * 函 数 名: lastwords_init_mem
 * 功能描述: 临终遗言内存初始化
 * 输入参数: void
 * 输出参数: void	
 * 返 回 值: 成功返回0，失败返回错误码
 * ****************************************** */
int lastwords_init_mem(void)
{
	/* 映射内存地址空间 */
	memaddr = ioremap_nocache(phyaddr, memsize);
	if (NULL == memaddr) {
		pr_err("failed to allocate char dev region\n");
		return -ENOMEM;
	}	

	pr_info("mem phyaddr = 0x%x, size = 0x%x, "
				"viraddr = 0x%p\n", phyaddr, memsize, memaddr);

	return 0;
}

/* ******************************************
 * 函 数 名: lastwords_exit_mem
 * 功能描述: 临终遗言内存去初始化
 * 输入参数: void
 * 输出参数: void	
 * 返 回 值: void
 * ****************************************** */
void lastwords_exit_mem(void)
{
	/* 解除内存映射 */
	if (memaddr) {
		iounmap(memaddr);
		memaddr = NULL;
	}
}

