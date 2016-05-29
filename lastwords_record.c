#include <linux/module.h>  
#include <linux/kernel.h>  
#include <linux/types.h>
#include <linux/string.h>
#include <linux/vmalloc.h>

#include "lastwords.h"
#include "lastwords_record.h"
#include "lastwords_mem.h"

struct lastwords_head {
	__u16 lh_magic;				/* 魔数 */
	__u16 lh_ecc;				/* ECC校验码 */
	__u32 lh_len;				/* 记录信息长度(包括消息头) */
	__u32 reserved;				/* 保留备用 */
} __attribute__((aligned(4)));

struct lastwords_attr {
	__u16 type;					/* 记录数据类型 */
	__u16 len;					/* 记录数据长度(包括属性头) */
} __attribute__((aligned(4)));

/* 记录数据魔数 */
#define LWRECD_MAGIC_NUM		0x5818

/* 单条属性记录上限 */
#define LWRECD_ATTR_SIZE_MAX	2048

/* 记录数据结构处理宏 */
#define LWRECD_ALIGNTO		4U
#define LWRECD_ALIGN(len)	(((len)+LWRECD_ALIGNTO-1) & ~(LWRECD_ALIGNTO-1))	/* 数据对齐 */

#define LWRECD_HDRLEN		((int) LWRECD_ALIGN(sizeof(struct lastwords_head)))	/* 数据头占用长度 */
#define LWRECD_LENGTH(len)	((len) + LWRECD_HDRLEN)								/* 数据总长(包括数据头) */
#define LWRECD_DATA(lwh)	((void*)(((char*)lwh) + LWRECD_HDRLEN))				/* 获取属性载荷地址 */
#define LWRECD_NEXT(lwh)	((void *)(((char*)(lwh)) + LWRECD_ALIGN((lwh)->lh_len)))	/* 获取属性尾 */
#define LWRECD_PAYLOAD(lwh)	((lwh)->lh_len - LWRECD_HDRLEN)						/* 数据载荷长度 */

#define LWA_HDRLEN			((int) LWRECD_ALIGN(sizeof(struct lastwords_attr)))	/* 属性头长度 */
#define LWA_LENGTH(len)		((len) + LWA_HDRLEN)								/* 属性总长(包括属性头) */
#define LWA_DATA(lwa)		((void *)((char*)(lwa) + LWA_HDRLEN))				/* 获取属性载荷首地址 */
#define LWA_CDATA(lwa,len)	((void *)((char*)(lwa) + LWA_HDRLEN + len))			/* 获取属性载荷指定地址 */
#define LWA_PAYLOAD(len)	(len - LWA_HDRLEN)									/* 属性载荷长度 */


static char *record_name[LAST_WORDS_ATTR_MAX] = {
		"attr_user_info",				/* LAST_WORDS_ATTR_USER */
		"attr_net_addr_info",			/* LAST_WORDS_ATTR_NETADR */
		"attr_module_info",				/* LAST_WORDS_ATTR_MODULE */
		"attr_reboot_info",				/* LAST_WORDS_ATTR_REBOOT */
		"attr_panic_info",				/* LAST_WORDS_ATTR_PANIC */
};

static void __iomem *record_base = NULL;
static int record_size = 0;
static struct lastwords_attr *pcur_lwa = NULL;
static DEFINE_SPINLOCK(record_lock);


/* ******************************************
 * 函 数 名: lastwords_format_record
 * 功能描述: 临终遗言格式化数据记录区
 * 输入参数: void
 * 输出参数: void	
 * 返 回 值: void
 * ****************************************** */
static void lastwords_format_record(void)
{
	struct lastwords_head lwh = {0};
	unsigned long flag = 0;

	spin_lock_irqsave(&record_lock, flag);
	
	/* 清空数据区 */
	lastwords_clean_mem();

	/* 格式化数据区，创建数据头 */
	lwh.lh_magic = LWRECD_MAGIC_NUM;
	lwh.lh_len = LWRECD_HDRLEN;
	lwh.lh_ecc = 0;			/* todo */

	memcpy(record_base, &lwh, sizeof(lwh));

	spin_unlock_irqrestore(&record_lock, flag);
}

/* ******************************************
 * 函 数 名: lastwords_format_record
 * 功能描述: 临终遗言格式化数据记录区
 * 输入参数: void
 * 输出参数: void	
 * 返 回 值: void
 * ****************************************** */
static void lastwords_delete_top_record(void)
{
	struct lastwords_head *plwh = record_base;
	struct lastwords_attr *plwa = NULL, *plwan = NULL;
	int lw_len = 0, lwa_len = 0;

	lw_len = LWRECD_PAYLOAD(plwh);
	plwa = (struct lastwords_attr *) LWRECD_DATA(plwh);
	
	if (0 == lw_len) 
		return;

	lwa_len = LWRECD_ALIGN(plwa->len);
	if (lw_len > lwa_len) {
		plwan = (struct lastwords_attr *)((char *)plwa + lwa_len);
		lw_len -= lwa_len;
		memmove(plwa, plwan, lw_len);
	}

	plwh->lh_len -= lwa_len;
	plwh->lh_ecc = 0;	/* to do */

	if (NULL != pcur_lwa)
		pcur_lwa = (struct lastwords_attr *)((char *)pcur_lwa - lwa_len);

	return;
}

/* ******************************************
 * 函 数 名: lastwords_prepare_attr
 * 功能描述: 临终遗言准备开始写数据内容(初始化头部)
 * 输入参数: lw_attr_t attr_type: 属性类型
 * 输出参数: void	
 * 返 回 值: void
 * ****************************************** */
void lastwords_prepare_attr(lw_attr_t attr_type)
{
	struct lastwords_head *plwh = record_base;
	struct lastwords_attr *plwa = NULL;
	unsigned long flag = 0;

	spin_lock_irqsave(&record_lock, flag);

	/* 若内存空间不足,则循环删除最初的记录项 */
	while ((plwh->lh_len + LWA_HDRLEN) > record_size)
		lastwords_delete_top_record();

	/* 初始化记录项头并更新数据头长度 */
	plwa = (struct lastwords_attr *) LWRECD_NEXT(plwh);
	plwa->type = attr_type;
	plwa->len = LWA_HDRLEN;

	plwh->lh_len += LWRECD_ALIGN(plwa->len);

	/* 保存当前记录头 */
	pcur_lwa = plwa;
	
	spin_unlock_irqrestore(&record_lock, flag);
}

/* ******************************************
 * 函 数 名: lastwords_prepare_attr
 * 功能描述: 临终遗言准备开始写数据内容(初始化头部)
 * 输入参数: lw_attr_t attr_type: 属性类型
 * 输出参数: void	
 * 返 回 值: void
 * ****************************************** */
int lastwords_print_attr(const char *fmt, ...)
{
	struct lastwords_head *plwh = record_base;
	struct lastwords_attr *plwa = NULL;	
	int payload = 0;
	int size = LWRECD_ATTR_SIZE_MAX;
	char *buf = NULL;	
	va_list args;
	unsigned long flag = 0;
	int ret = 0;

	spin_lock_irqsave(&record_lock, flag);

	if (NULL == pcur_lwa) {
		pr_err("Lastwords current attr is not prepared\n");
		ret = 0;
		goto out;
	}

	buf = vzalloc(size);
	if (NULL == buf) {
		ret = -ENOMEM;
		goto out;
	}
	
	va_start(args, fmt);
	size = vscnprintf(buf, size, fmt, args);
	va_end(args);
	
	/* add '\0' end */
	size++;	

	/* 数据长度保护 */
	if ((LWRECD_HDRLEN + LWRECD_ALIGN(LWA_LENGTH(size))) > record_size) {
		pr_err("Lastword record attr size = %d too large!\n", size);
		vfree(buf);
		ret = -ENOBUFS;
		goto out;		
	}

	/* 若内存空间不足,则循环删除最初的记录项 */
	while ((plwh->lh_len + LWRECD_ALIGN(LWA_LENGTH(size))) > record_size)
		lastwords_delete_top_record();

	/* 写入数据 */
	plwa = pcur_lwa;
	payload = LWA_PAYLOAD(plwa->len);
	memcpy(LWA_CDATA(plwa,payload), buf, size);

	plwh->lh_len -= LWRECD_ALIGN(plwa->len);
	plwa->len += size;
	plwh->lh_len += LWRECD_ALIGN(plwa->len);

	vfree(buf);
	ret = size;

out:
	spin_unlock_irqrestore(&record_lock, flag);
	return ret;
}

/* ******************************************
 * 函 数 名: lastwords_end_record
 * 功能描述: 临终遗言结束一次写数据内容
 * 输入参数: void
 * 输出参数: void	
 * 返 回 值: void
 * ****************************************** */
void lastwords_end_attr(void)
{
	struct lastwords_head *plwh = record_base;
	unsigned long flag = 0;

	spin_lock_irqsave(&record_lock, flag);	
	
	plwh->lh_ecc = 0;	/* to do */
	pcur_lwa = NULL;
	
	spin_unlock_irqrestore(&record_lock, flag);
}

/* ******************************************
 * 函 数 名: lastwords_write_record
 * 功能描述: 临终遗言完成一次完整的写数据内容
 * 输入参数: lw_attr_t attr_type: 属性类型
 			 char *buf: 属性内容
 			 int len: 属性长度
 * 输出参数: void	
 * 返 回 值: 成功返回写入长度，失败返回错误码
 * ****************************************** */
int lastwords_write_record(lw_attr_t attr_type, char *buf, int len)
{
	struct lastwords_head *plwh = record_base;
	struct lastwords_attr *plwa = NULL;
	unsigned long flag = 0;
	int ret;

	spin_lock_irqsave(&record_lock, flag);	

	if ((LWRECD_HDRLEN + LWRECD_ALIGN(LWA_LENGTH(len))) > record_size) {
		pr_err("Lastword record attr size = %d too large!\n", len);
		ret = -ENOBUFS;
		goto out;
	}

	/* 若内存空间不足,则循环删除最初的记录项 */
	while ((plwh->lh_len + LWRECD_ALIGN(LWA_LENGTH(len))) > record_size)
		lastwords_delete_top_record();
	
	plwa = (struct lastwords_attr *) LWRECD_NEXT(plwh);
	plwa->type = attr_type;
	plwa->len = LWA_LENGTH(len);

	plwh->lh_len += LWRECD_ALIGN(plwa->len);
	plwh->lh_ecc = 0;	/* to do */

	/* 为防内存对齐错误，一次写入内存数据空间 */
	memcpy(LWA_DATA(plwa), buf, len);
	ret = len;

out:
	spin_unlock_irqrestore(&record_lock, flag);
	return ret;
}

/* ******************************************
 * 函 数 名: lastwords_print_record
 * 功能描述: 临终遗言完成一次完整写数据内容
 * 输入参数: lw_attr_t attr_type: 属性类型
 			 const char *fmt: 属性内容
 * 输出参数: void	
 * 返 回 值: int 写入长度
 * ****************************************** */
int lastwords_print_record(lw_attr_t attr_type, const char *fmt, ...)
{
	char *buf = NULL;
	int size = LWRECD_ATTR_SIZE_MAX;
	va_list args;
	int ret;

	buf = vzalloc(size);
	if (NULL == buf)
		return -ENOMEM;
	
	va_start(args, fmt);
	size = vscnprintf(buf, size, fmt, args);
	va_end(args);

	ret = lastwords_write_record(attr_type, buf, size+1);

	vfree(buf);

	return ret;
}

/* ******************************************
 * 函 数 名: lastwords_show_head
 * 功能描述: 临终遗言打印数据头部内容
 * 输入参数: void
 * 输出参数: void
 * 返 回 值: void
 * ****************************************** */
void lastwords_show_head(void)
{
	struct lastwords_head *plwh = record_base;

	pr_info("/============Lastwords Heard Info============/\n");
	pr_info("Magic Num = 0x%x\n", plwh->lh_magic);
	pr_info("ECC Value= 0x%x\n", plwh->lh_ecc);
	pr_info("Record Len = 0x%x\n", plwh->lh_len);
	pr_info("/======================================/\n");

}

/* ******************************************
 * 函 数 名: lastwords_show_record
 * 功能描述: 临终遗言打印数据内容
 * 输入参数: lw_attr_t attr_type: 属性类型
 * 输出参数: void	
 * 返 回 值: void
 * ****************************************** */
void lastwords_show_record(lw_attr_t attr_type)
{
	struct lastwords_head *plwh = record_base;
	struct lastwords_attr *plwa = NULL;	
	int len = 0, lw_len = 0;

	lw_len = LWRECD_PAYLOAD(plwh);
	plwa = (struct lastwords_attr *) LWRECD_DATA(plwh);

	/* 输出头部信息 */
	lastwords_show_head();

	/* 循环输出指定属性的记录信息 */
	while (len < lw_len) {
		len += LWRECD_ALIGN(plwa->len);
		if (plwa->type == attr_type || __LAST_WORDS_ATTR_MAX == attr_type) {
			pr_info("/-------------%s-------------/\n", record_name[plwa->type-1]);
			pr_info("Attr Type = %d\n", plwa->type);
			pr_info("Attr Len = 0x%x\n\n", plwa->len);
			pr_info("%s\n", (char *) LWA_DATA(plwa));
			pr_info("/--------------------------------/\n");
		}
		plwa = (struct lastwords_attr *) (LWRECD_DATA(plwh) + len);
	}
}




/* ******************************************
 * 函 数 名: lastwords_init_record
 * 功能描述: 临终遗言初始化数据存储管理
 * 输入参数: void
 * 输出参数: void	
 * 返 回 值: 成功返回0，失败返回错误码
 * ****************************************** */
int lastwords_init_record(void)
{
	struct lastwords_head *plwh = NULL;
	
	/* 1、获取内存基地址和长度 */
	record_base = lastwords_get_membase();
	record_size = lastwords_get_memsize();

	if (NULL == record_base || LWRECD_HDRLEN > record_size) {
		pr_err("Lastwords do not have enough record memory\n");
		return -ENOMEM;
	}

	/* 2、格式化存储空间(若已存在则不格式化) */
	plwh = (struct lastwords_head *) record_base;
	
	if (LWRECD_MAGIC_NUM != plwh->lh_magic) 
		lastwords_format_record();
	
	return 0;
}

/* ******************************************
 * 函 数 名: lastwords_exit_record
 * 功能描述: 临终遗言去初始化数据存储管理
 * 输入参数: void
 * 输出参数: void	
 * 返 回 值: void
 * ****************************************** */
void lastwords_exit_record(void)
{

}

