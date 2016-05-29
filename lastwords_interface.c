#include <linux/module.h>  
#include <linux/kernel.h>  
#include <linux/types.h>
#include <linux/vmalloc.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#include <asm/uaccess.h>

#include "lastwords.h"
#include "lastwords_record.h"

struct proc_dir_entry *lw_proc = NULL;


/* ******************************************
 * 函 数 名: lastwords_show_proc
 * 功能描述: 临终遗言show接口
 * 输入参数: struct seq_file *seq: seq文件描述符
 			 void *offset: 用户数据偏移
 * 输出参数: void	
 * 返 回 值: 返回读取字节数
 * ****************************************** */
static int lastwords_show_proc(struct seq_file *seq, void *offset)
{
	int ret = 0;
	/* to do */
	return ret;
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

	/* 根据输入进行对应的处理 */
	if ('r' == kbuf[0]) 
		lastwords_show_record(__LAST_WORDS_ATTR_MAX);
	else if ('w' == kbuf[0] && ' ' == kbuf[1])
		ret = lastwords_write_user(kbuf+2, len-2);
 	else 
		pr_info("User input: %s\n", kbuf);
		
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
	int ret = 0;

	if (!try_module_get(THIS_MODULE))
		return -ENODEV;

	ret = single_open(file, lastwords_show_proc, NULL);
	if (ret)
		module_put(THIS_MODULE);
	return ret;
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
	int res = single_release(inode, file);
	module_put(THIS_MODULE);
	return res;
}

static const struct file_operations lastwords_proc_fops = {
	.open		= lastwords_open_proc,
	.read		= seq_read,
	.write		= lastwords_write_proc,
	.llseek		= seq_lseek,
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
 * 函 数 名: lastwords_init_interface
 * 功能描述: 临终遗言用户接口初始化
 * 输入参数: void
 * 输出参数: void	
 * 返 回 值: 成功返回0，失败返回错误码
 * ****************************************** */
int lastwords_init_interface(void)
{
	/* 初始化proc接口 */
	lastwords_init_proc();

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
	/* 删除proc接口 */
	lastwords_exit_proc();
}