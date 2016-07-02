#include <linux/module.h>  
#include <linux/kernel.h>  
#include <linux/types.h>
#include <linux/string.h>
#include <linux/vmalloc.h>

#include "lastwords.h"
#include "lastwords_record.h"
#include "lastwords_mem.h"

/* 记录数据魔数 */
#define LWRECD_MAGIC_NUM		0x5818

/* 单条属性记录上限 */
#define LWRECD_ATTR_SIZE_MAX	2048

/* 记录数据结构处理宏 */
#define LWRECD_ALIGNTO		4U
#define LWRECD_ALIGN(len)	(((len)+LWRECD_ALIGNTO-1) & ~(LWRECD_ALIGNTO-1))	/* 数据对齐 */

#define LWRECD_HDRLEN		((int) LWRECD_ALIGN(sizeof(struct lws_head)))		/* 数据头占用长度 */
#define LWRECD_LENGTH(len)	((len) + LWRECD_HDRLEN)								/* 数据总长(包括数据头) */
#define LWRECD_DATA(lwh)	((void *)(((char*)lwh) + LWRECD_HDRLEN))			/* 获取属性载荷地址 */
#define LWRECD_NEXT(lwh)	((void *)(((char*)(lwh)) + LWRECD_ALIGN((lwh)->lh_len)))	/* 获取属性尾 */
#define LWRECD_PAYLOAD(lwh)	((lwh)->lh_len - LWRECD_HDRLEN)						/* 数据载荷长度 */

#define LWA_HDRLEN			((int) LWRECD_ALIGN(sizeof(struct lws_attr)))		/* 属性头长度 */
#define LWA_LENGTH(len)		((len) + LWA_HDRLEN)								/* 属性总长(包括属性头) */
#define LWA_DATA(lwa)		((void *)((char*)(lwa) + LWA_HDRLEN))				/* 获取属性载荷首地址 */
#define LWA_PAYLOAD(len)	(len - LWA_HDRLEN)									/* 属性载荷长度 */

static void __iomem *record_base = NULL;
static int record_size = 0;
static DEFINE_SPINLOCK(record_lock);


/* ******************************************
 * 函 数 名: lastwords_prepare_attr
 * 功能描述: 临终遗言准备开始写数据内容(初始化头部)
 * 输入参数: __u16 attr_type: 属性类型
 * 输出参数: void	
 * 返 回 值: struct lws_attr * 返回新生成的attr头部
 * ****************************************** */
struct lws_attr * lastwords_prepare_attr(__u16 attr_type)
{
	struct lws_head *plwh = record_base;
	struct lws_attr *plwa = NULL;
	unsigned long flag = 0;

	spin_lock_irqsave(&record_lock, flag);	

	/* 数据长度保护 */
	if ((plwh->lh_len + LWA_HDRLEN) > record_size) {
		pr_err("Lastwords record mem not enough\n");
		goto out;
	}

	/* 初始化记录项头并更新数据头长度 */
	plwa = (struct lws_attr *) LWRECD_NEXT(plwh);
	plwa->type = attr_type;
	plwa->len = LWA_HDRLEN;

	plwh->lh_len += LWRECD_ALIGN(plwa->len);
	
out:
	spin_unlock_irqrestore(&record_lock, flag);
	return plwa;
}

/* ******************************************
 * 函 数 名: lastwords_print
 * 功能描述: 临终遗言准备开始写数据内容(初始化头部)
 * 输入参数: lw_attr_t attr_type: 属性类型
 * 输出参数: void	
 * 返 回 值: int 写入的字节数
 * ****************************************** */
int lastwords_print(const char *fmt, ...)
{
	struct lws_head *plwh = record_base;
	__u32 size = LWRECD_ATTR_SIZE_MAX;
	unsigned long flag = 0;
	char *buf = NULL;	
	va_list args;
	int ret = 0;

	buf = vzalloc(size);
	if (NULL == buf) 
		return -ENOMEM;
	
	va_start(args, fmt);
	size = vscnprintf(buf, size, fmt, args);
	va_end(args);
	
	/* add '\0' end */
	//size++;	

	spin_lock_irqsave(&record_lock, flag);	

	/* 数据长度保护 */
	if ((plwh->lh_len + LWRECD_ALIGN(size)) > record_size) {
		pr_err("Lastwords record mem not enough\n");
		ret = -ENOMEM;
		goto out;
	}

	/* 写入数据 */
	memset(((char*)plwh + plwh->lh_len), 0, size);
	memcpy(((char*)plwh + plwh->lh_len), buf, size);

	plwh->lh_len += size;
	ret = size;

out:
	spin_unlock_irqrestore(&record_lock, flag);
	vfree(buf);
	return ret;
}

/* ******************************************
 * 函 数 名: lastwords_end_record
 * 功能描述: 临终遗言结束一次写数据内容
 * 输入参数: struct lws_attr *plwa: 属性头
 * 输出参数: void	
 * 返 回 值: void
 * ****************************************** */
void lastwords_end_attr(struct lws_attr *plwa)
{
	struct lws_head *plwh = record_base;	
	char *p = NULL;
	__u32 payload = 0;
	unsigned long flag = 0;	
	int i = 0;

	spin_lock_irqsave(&record_lock, flag);	

	/* 更新长度信息 */
	plwa->len = ((char*)plwh + plwh->lh_len) - (char *)plwa;

	/* 将本attr中的'\0'转换为'\n'(不包含最后的'\0') */
	payload = LWA_PAYLOAD(plwa->len);
	p = (char*)LWA_DATA(plwa);

	pr_debug("attr payload=%d\n", payload);
	
	if (0 < payload && LAST_WORDS_RECD_UNSPEC < plwa->type &&
			__LAST_WORDS_RECD_MAX > plwa->type) {
		for (i = 0; i < payload-1; i++) {
			*p = ('\0' == *p) ? ' ' : *p;
			p++;
		}
		*p = '\0';
	}

	/* 更新参数 */
	plwh->lh_len = LWRECD_ALIGN(plwh->lh_len);
	plwh->lh_ecc = 0;	/* to do */
	
	spin_unlock_irqrestore(&record_lock, flag);
}

#if 0
/* ******************************************
 * 函 数 名: lastwords_write_record
 * 功能描述: 临终遗言完成一次完整的写数据内容
 * 输入参数: __u16 attr_type: 属性类型
 			 char *buf: 属性内容
 			 __u16 len: 属性长度
 * 输出参数: void	
 * 返 回 值: 成功返回写入长度，失败返回错误码
 * ****************************************** */
int lastwords_write_record(__u16 attr_type, char *buf, __u16 len)
{
	struct lws_head *plwh = record_base;
	struct lws_attr *plwa = NULL;
	unsigned long flag = 0;	
	int ret;

	spin_lock_irqsave(&record_lock, flag);	

	/* 数据长度保护 */
	if ((plwh->lh_len + LWRECD_ALIGN(len)) > record_size) {
		pr_err("Lastwords record mem not enough\n");
		ret = -ENOMEM;
		goto out;
	}

	plwa = (struct lws_attr *) LWRECD_NEXT(plwh);
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
int lastwords_print_record(__u16 attr_type, const char *fmt, ...)
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
#endif




/* ******************************************
 * 函 数 名: lastwords_format_record
 * 功能描述: 临终遗言格式化数据记录区
 * 输入参数: void
 * 输出参数: void	
 * 返 回 值: void
 * ****************************************** */
void lastwords_format_record(void)
{
	struct lws_head lwh = {0};
	unsigned long flag = 0;
	
	spin_lock_irqsave(&record_lock, flag);	
	
	/* 清空数据区 */
	lastwords_clean_mem();

	/* 格式化数据区，创建数据头 */
	lwh.lh_magic	= LWRECD_MAGIC_NUM;
	lwh.lh_len		= LWRECD_HDRLEN;
	lwh.lh_ecc		= 0;			/* to do */

	memcpy(record_base, &lwh, sizeof(lwh));

	spin_unlock_irqrestore(&record_lock, flag);
}

/* ******************************************
 * 函 数 名: lastwords_dump_record
 * 功能描述: 临终遗言获取所有记录信息
 * 输入参数: __u32 len: 缓存大小
 			 __u32 off: 偏移长度
 * 输出参数: char *buf: 缓存空间
 * 返 回 值: __u32: 返回获取长度
 * ****************************************** */
__u32 lastwords_dump_record(char *buf, __u32 len, __u32 off)
{
	struct lws_head *plwh = record_base;
	__u32 recordlen = 0;
	unsigned long flag = 0;
	
	spin_lock_irqsave(&record_lock, flag);	
	
	recordlen = plwh->lh_len;
	if ((off + len) > recordlen) {
		spin_unlock_irqrestore(&record_lock, flag);
		return 0;
	}
	
	len = (len < (recordlen - off)) ? len : (recordlen - off);
	memcpy(buf, record_base + off, len);

	spin_unlock_irqrestore(&record_lock, flag);
	return len;
}

/* ******************************************
 * 函 数 名: lastwords_dump_record
 * 功能描述: 临终遗言获取所有记录信息
 * 输入参数: __u32 len: 缓存大小
 * 输出参数: char *buf: 缓存空间
 * 返 回 值: void: 成功返回0
 * ****************************************** */
__u32 lastwords_get_recordlen(void)
{
	struct lws_head *plwh = record_base;	
	__u32 record_len = 0;	
	unsigned long flag = 0;
	
	spin_lock_irqsave(&record_lock, flag);	
	record_len = plwh->lh_len;
	spin_unlock_irqrestore(&record_lock, flag);

	return record_len;
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
	struct lws_head *plwh = NULL;
	
	/* 1、获取内存基地址和长度 */
	record_base = lastwords_get_membase();
	record_size = lastwords_get_memsize();

	if (NULL == record_base || LWRECD_HDRLEN > record_size) {
		pr_err("Lastwords do not have enough record memory\n");
		return -ENOMEM;
	}

	/* 2、格式化存储空间(若已存在则不格式化) */
	plwh = (struct lws_head *) record_base;
	
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
	/* to do */
}

