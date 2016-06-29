#include <linux/module.h>  
#include <linux/kernel.h>  
#include <linux/types.h>
#include <linux/vmalloc.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#include <asm/uaccess.h>

#include "lastwords.h"
#include "lastwords_mem.h"
#include "lastwords_record.h"
#include "lastwords_export.h"
#include "lastwords_interface.h"


/* 用户输出类型 */
#define LASTWORDS_USER_EXPORT			( LASTWORDS_EXPORT_TIME \
										| LASTWORDS_EXPORT_SYSINFO \
										| LASTWORDS_EXPORT_DMESG \
										| LASTWORDS_EXPORT_PS )

static struct proc_dir_entry *lw_proc = NULL;


/* ******************************************
 * 函 数 名: lastwords_write_user
 * 功能描述: 临终遗言用户触发接口
 * 输入参数: char *buf: 用户触发消息
 			 size_t len: 用户传入的数据长度
 * 输出参数: void	
 * 返 回 值: int: 成功返回0
 * ****************************************** */
static int lastwords_write_user(char *data)
{
	return lastwords_export_attr(LAST_WORDS_ATTR_USER, LASTWORDS_USER_EXPORT, data);
}

/* ******************************************
 * 函 数 名: lastwords_write_proc
 * 功能描述: 临终遗言写入proc接口
 * 输入参数: struct file *file: proc文件描述符
 			 const char __user *buffer: 用户传入的数据缓存
 			 size_t len: 用户传入的数据长度
 			 loff_t *offset: 用户数据偏移
 * 输出参数: void	
 * 返 回 值: 返回写入字节数
 * ****************************************** */
static ssize_t lastwords_write_proc(struct file *file, const char __user *buffer,
									size_t len, loff_t *offset)
{
	ssize_t ret, res;
	char *kbuf = NULL;

	kbuf = vzalloc(len);
	if (NULL == kbuf) 
		return -ENOMEM;

	res = copy_from_user(kbuf, buffer, len);
	if (res == len)
		return -EFAULT;

	/* 触发用户记录 */
	ret = lastwords_write_user(kbuf);	
	if (ret)
		return -EFAULT;
	
	ret = len;
	vfree(kbuf);
	
	return ret;
}

/* ******************************************
 * 函 数 名: lastwords_open_proc
 * 功能描述: 临终遗言打开proc接口
 * 输入参数: struct inode *inode: proc文件索引节点
 			 struct file *file: proc文件描述符
 * 输出参数: void	
 * 返 回 值: 成功返回0，失败返回错误码
 * ****************************************** */
static int lastwords_open_proc(struct inode *inode, struct file *file)
{
	if (!try_module_get(THIS_MODULE))
		return -ENODEV;

	return 0;
}

/* ******************************************
 * 函 数 名: lastwords_release_proc
 * 功能描述: 临终遗言关闭proc接口
 * 输入参数: struct inode *inode: proc文件索引节点
 			 struct file *file: proc文件描述符
 * 输出参数: void	
 * 返 回 值: 成功返回0，失败返回错误码
 * ****************************************** */
static int lastwords_release_proc(struct inode *inode, struct file *file)
{
	module_put(THIS_MODULE);
	return 0;
}

static const struct file_operations lastwords_proc_fops = {
	.open		= lastwords_open_proc,
	.write		= lastwords_write_proc,
	.release	= lastwords_release_proc,
};


/* ******************************************
 * 函 数 名: lastwords_proc_init
 * 功能描述: 临终遗言用户Proc接口初始化
 * 输入参数: void
 * 输出参数: void	
 * 返 回 值: void
 * ****************************************** */
void lastwords_init_proc(void)
{
	/* 创建 lastwords proc 目录 */
	lw_proc = proc_create_data("lastwords", 0, NULL, &lastwords_proc_fops, NULL);
}

/* ******************************************
 * 函 数 名: lastwords_proc_init
 * 功能描述: 临终遗言用户Proc接口初始化
 * 输入参数: void
 * 输出参数: void	
 * 返 回 值: void
 * ****************************************** */
void lastwords_exit_proc(void)
{
	proc_remove(lw_proc);
}






/* ******************************************
 * 函 数 名: lastwords_write_dev
 * 功能描述: 临终遗言用户字符设备控制
 * 输入参数: struct file *file: 文件描述
 			 unsigned int cmd: 命令字
 			 unsigned long arg: 命令参数
 * 输出参数: void	
 * 返 回 值: long: 成功返回0，失败返回错误码
 * ****************************************** */
static long lastwords_ioctl_dev(struct file *file, unsigned int cmd, 
					unsigned long arg)
{
	int ret = 0;
	
	switch (cmd) {
	/* 获取记录信息的总长度 */
	case LASTWORDS_GET_MEMSIZE: {
		__u32 len = 0;
		
		len = lastwords_get_recordlen();
		ret = put_user(len, (unsigned int __user *)arg);
		break;
	}
	/* 开关临终遗言功能 */
	case LASTWORDS_CTRL_ONOFF: {
		/* to do */		
		break;
	}
	/* 格式化内存空间 */
	case LASTWORDS_FORMAT_MEM: {
		lastwords_export_clear();
		break;
	}	
	/* 触发用户记录 */
	case LASTWORDS_TRIGGER_RECORD: {
		/* to do */		
		break;
	}	
	default:
		pr_err("Lastwords unknown ioctl cmd = 0x%x\n", cmd);
	}
	
	return ret;
}

#if 1
/* ******************************************
 * 函 数 名: lastwords_mmap_dev
 * 功能描述: 临终遗言用户字符设备映射内存
 * 输入参数: struct file *file: 文件描述
 			 struct vm_area_struct *vm: 内存结构
 * 输出参数: void	
 * 返 回 值: int: 成功返回0，失败返回错误码
 * ****************************************** */
int lastwords_mmap_dev(struct file *file, struct vm_area_struct *vma)
{
	phys_addr_t phyaddr = 0;
	int ret = 0;

	phyaddr = lastwords_get_memphy();
	
	ret = remap_pfn_range(vma, vma->vm_start, phyaddr >> PAGE_SHIFT,
						vma->vm_end - vma->vm_start, vma->vm_page_prot);
	if (ret)
		pr_err("Lastwords remap pfn range fail (err=%d)\n", ret);
	
	return ret;
}
#endif

/* ******************************************
 * 函 数 名: lastwords_write_dev
 * 功能描述: 临终遗言用户字符设备读取接口
 * 输入参数: struct file *file: 文件描述
 			 char __user *buf: 用户内存空间缓存
 			 size_t len: 读取大小
 			 loff_t *off: 偏移
 * 输出参数: void	
 * 返 回 值: long: 成功返回0，失败返回错误码
 * ****************************************** */
ssize_t lastwords_read_dev(struct file *file, char __user *buf, size_t len, loff_t *off)
{
	char *kbuf = NULL;
	__u32 read_len = 0;

	if (0 == len)
		return 0;
	
	kbuf = vmalloc(len);
	if (NULL == kbuf) {
		pr_err("Lastwords vmalloc error\n");
		return -ENOMEM;
	}

	/* 获取临终遗言记录数据 */
	read_len = lastwords_export_dump(kbuf, (__u32)len, *off);
	if (0 > read_len) {
		vfree(kbuf);
		return read_len;
	}

	/* 拷贝至用户空间 */
	if (copy_to_user(buf, kbuf, read_len)) 
		pr_err("Lastwords copy to user error\n");

	vfree(kbuf);
	*off += read_len;
	
	return read_len;
}

static const struct file_operations lastwords_dev_fops = {
	.owner		= THIS_MODULE,
	.llseek		= default_llseek,
	.read		= lastwords_read_dev,
	.mmap		= lastwords_mmap_dev,
	.unlocked_ioctl	= lastwords_ioctl_dev,
};

static struct miscdevice lastwords_miscdev = {
	.minor		= MISC_DYNAMIC_MINOR,
	.name		= "lastwords",
	.fops		= &lastwords_dev_fops,
};

/* ******************************************
 * 函 数 名: lastwords_init_dev
 * 功能描述: 临终遗言用户字符设备初始化
 * 输入参数: void
 * 输出参数: void	
 * 返 回 值: int: 成功返回0，失败返回错误码
 * ****************************************** */
int lastwords_init_dev(void)
{	
	int ret = 0;
	
	/* 创建字符设备 */
	ret = misc_register(&lastwords_miscdev);
	if (ret) 
		pr_err("cannot register lastwords miscdev (err=%d)\n", ret);

	return ret;
}

/* ******************************************
 * 函 数 名: lastwords_proc_init
 * 功能描述: 临终遗言用户字符设备去初始化
 * 输入参数: void
 * 输出参数: void	
 * 返 回 值: void
 * ****************************************** */
void lastwords_exit_dev(void)
{
	misc_deregister(&lastwords_miscdev);
}




/* ******************************************
 * 函 数 名: lastwords_init_interface
 * 功能描述: 临终遗言用户接口初始化
 * 输入参数: void
 * 输出参数: void	
 * 返 回 值: 成功返回0，失败返回错误码
 * ****************************************** */
int lastwords_init_interface(void)
{
	int ret = 0;
	
	/* 初始化proc接口 */
	lastwords_init_proc();
	
	/* 注册字符设备 */
	ret = lastwords_init_dev();
	if (ret) {
		lastwords_exit_proc();
		return ret;
	}
	
	return 0;
}

/* ******************************************
 * 函 数 名: lastwords_exit_interface
 * 功能描述: 临终遗言用户接口去初始化
 * 输入参数: void
 * 输出参数: void	
 * 返 回 值: void
 * ****************************************** */
void lastwords_exit_interface(void)
{
	/* 删除字符设备 */
	lastwords_exit_dev();
	
	/* 删除proc接口 */
	lastwords_exit_proc();
}