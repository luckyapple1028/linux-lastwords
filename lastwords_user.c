#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include "lastwords_user.h"


#define LW_INPUT_SIZE		128			/* 用户输入缓存 */

/* 记录数据结构处理宏 */
#define LWRECD_ALIGNTO		4U
#define LWRECD_ALIGN(len)	(((len)+LWRECD_ALIGNTO-1) & ~(LWRECD_ALIGNTO-1))	/* 数据对齐 */

#define LWRECD_HDRLEN		((int) LWRECD_ALIGN(sizeof(struct lws_head)))	/* 数据头占用长度 */
#define LWRECD_LENGTH(len)	((len) + LWRECD_HDRLEN)							/* 数据总长(包括数据头) */
#define LWRECD_DATA(lwh)	((void *)(((char*)lwh) + LWRECD_HDRLEN))		/* 获取属性载荷地址 */
#define LWRECD_NEXT(lwh)	((void *)(((char*)(lwh)) + LWRECD_ALIGN((lwh)->lh_len)))	/* 获取属性尾 */
#define LWRECD_PAYLOAD(lwh)	((lwh)->lh_len - LWRECD_HDRLEN)					/* 数据载荷长度 */

#define LWA_HDRLEN			((int) LWRECD_ALIGN(sizeof(struct lws_attr)))	/* 属性头长度 */
#define LWA_LENGTH(len)		((len) + LWA_HDRLEN)							/* 属性总长(包括属性头) */
#define LWA_DATA(lwa)		((void *)((char*)(lwa) + LWA_HDRLEN))			/* 获取属性载荷首地址 */
#define LWA_PAYLOAD(len)	(len - LWA_HDRLEN)								/* 属性载荷长度 */

static char *lw_attrname[LAST_WORDS_ATTR_MAX] = {
		"User Trigger",				/* LAST_WORDS_ATTR_USER */
		"Net Addr Trigger",			/* LAST_WORDS_ATTR_NETADR */
		"Module Trigger",			/* LAST_WORDS_ATTR_MODULE */
		"Reboot Trigger",			/* LAST_WORDS_ATTR_REBOOT */
		"Panic Trigger",			/* LAST_WORDS_ATTR_PANIC */
		"Die Trigger",				/* LAST_WORDS_ATTR_DIE */
		"Oom Trigger"				/* LAST_WORDS_ATTR_OOM */	
};

static char *lw_rdname[LAST_WORDS_RECD_MAX] = {
		"Trigger info",				/* LAST_WORDS_RECD_TRIGGER */
		"Time info",				/* LAST_WORDS_RECD_TIME */
		"System info",				/* LAST_WORDS_RECD_SYSINFO */
		"Dmesg info",				/* LAST_WORDS_RECD_DMESG */
		"Ps info",					/* LAST_WORDS_RECD_PS */
		"Backtrace info"			/* LAST_WORDS_RECD_BACKTRACE */
};

char lw_tmp[LW_INPUT_SIZE] = {0};
int lw_fd = 0;


/* ******************************************
 * 函 数 名: lastwords_main_control
 * 功能描述: 临终遗言用户解析程序主循环
 * 输入参数: void
 * 输出参数: unsigned int *size: 临终遗言内存信息	
 * 返 回 值: int:成功返回0
 * ****************************************** */
int lastwords_get_memsize(unsigned int *size)
{
	int ret = 0;
	
	ret = ioctl(lw_fd, LASTWORDS_GET_MEMSIZE, size);
	if (ret) 
		printf("fail to ioctl LASTWORDS_GET_MEMSIZE cmd (%d)\n", errno);
	
	return ret; 
}

/* ******************************************
 * 函 数 名: lastwords_show_header
 * 功能描述: 临终遗言用户解析打印存储头部
 * 输入参数: void
 * 输出参数: void
 * 返 回 值: void
 * ****************************************** */
void lastwords_show_header(void)
{
	struct lws_head lwh;
	ssize_t ret = 0;

	(void)lseek(lw_fd, 0, SEEK_SET);
	
	/* 读取信息 */
	ret = read(lw_fd, &lwh, LWRECD_HDRLEN);
	if (ret != LWRECD_HDRLEN) {
		printf("fail to read lastwords heads\n");
		return;
	}

	printf("/============Lastwords Heard Info============/\n");
	printf("Magic Num = 0x%x\n", lwh.lh_magic);
	printf("ECC Value= 0x%x\n", lwh.lh_ecc);
	printf("Record Len = 0x%x\n", lwh.lh_len);
	printf("/======================================/\n");

}

/* ******************************************
 * 函 数 名: lastwords_show_header
 * 功能描述: 临终遗言用户解析打印所有信息
 * 输入参数: void
 * 输出参数: void
 * 返 回 值: void
 * ****************************************** */
void lastwords_show_all(void)
{
	struct lws_head lwh;
	struct lws_attr lwa;
	struct lws_attr *plwa = NULL;	
	struct lws_attr *plwa2 = NULL;	
	char *buf = NULL;
	unsigned int len = 0, lw_len = 0;
	unsigned int len2 = 0, la_len = 0;
	ssize_t ret = 0;

	(void)lseek(lw_fd, 0, SEEK_SET);

	/* 读取头部信息 */
	ret = read(lw_fd, &lwh, LWRECD_HDRLEN);
	if (ret != LWRECD_HDRLEN) {
		printf("fail to read lastwords heads\n");
		return;
	}

	/* 打印指定属性的记录信息 */
	printf("/============Lastwords Heard Info============/\n");
	printf("Magic Num = 0x%x\n", lwh.lh_magic);
	printf("ECC Value= 0x%x\n", lwh.lh_ecc);
	printf("Record Len = 0x%x\n", lwh.lh_len);
	printf("/======================================/\n");

	/* 循环打印指定属性的记录信息 */
	lw_len = LWRECD_PAYLOAD(&lwh);
	len = 0;
	while (len < lw_len) {
		/* 读取属性头，获取属性长度 */
		ret = read(lw_fd, &lwa, LWA_HDRLEN);
		if (ret != LWA_HDRLEN) {
			printf("fail to read lastwords attr heads\n");
			return;
		}

		buf = malloc(LWRECD_ALIGN(lwa.len));
		if (NULL == buf) {
			printf("fail to malloc\n");
			return;
		}

		memcpy(buf, &lwa, LWA_HDRLEN);
		ret = read(lw_fd, buf+LWA_HDRLEN, LWA_PAYLOAD(lwa.len));
		if (ret != LWA_PAYLOAD(lwa.len)) {
			printf("fail to read lastwords attr (need:0x%x true:0x%x)\n", 
						LWA_PAYLOAD(lwa.len), ret);
			free(buf);
			return;
		}

		plwa = (struct lws_attr *) buf;
		len += LWRECD_ALIGN(plwa->len);
		if (LAST_WORDS_ATTR_UNSPEC < plwa->type && 
				__LAST_WORDS_ATTR_MAX  > plwa->type) {
			printf("/-------------%s-------------/\n", lw_attrname[LAST_WORDS_ATTR_N(plwa->type)]);
			printf("Attr Type = %d\n", plwa->type);
			printf("Attr Len = 0x%x\n", plwa->len);
			printf("/--------------------------------/\n");

			la_len = LWA_PAYLOAD(plwa->len);
			len2 = 0;
			plwa2 = (struct lws_attr *) LWA_DATA(plwa);
			while (len2 < la_len) {
				len2 += LWRECD_ALIGN(plwa2->len);
				
				printf("/-------------%s-------------/\n", lw_rdname[LAST_WORDS_RECD_N(plwa2->type)]);
				printf("Attr Type = %d\n", plwa2->type);
				printf("Attr Len = 0x%x\n\n", plwa2->len);
				if (plwa2->len > LWA_HDRLEN)
					printf("%s\n", (char *) LWA_DATA(plwa2));
				
				plwa2 = (struct lws_attr *) ((char *)LWA_DATA(plwa) + len2);
			}
		} else {
			printf("/-------------%s-------------/\n", lw_attrname[LAST_WORDS_ATTR_N(plwa->type)]);
			printf("Attr Type = %d\n", plwa->type);
			printf("Attr Len = 0x%x\n\n", plwa->len);
			if (plwa->len > LWA_HDRLEN)
				printf("%s\n", (char *) LWA_DATA(plwa));
			
		}

		printf("/--------------------------------/\n");

		free(buf);
		buf = NULL;
	}
}

/* ******************************************
 * 函 数 名: lastwords_show_memhex
 * 功能描述: 临终遗言用户解析打印内存二进制数据
 * 输入参数: unsigned int addr: 起始地址
 * 输出参数: void
 * 返 回 值: void
 * ****************************************** */
void lastwords_show_memhex(unsigned int addr)
{
	unsigned int *data = NULL;
	int memsize = 0x100;
	int i = 0;

	data = mmap(NULL, memsize, PROT_READ, MAP_SHARED, lw_fd, 0);
	if (NULL == data) {
		printf("fail to mmap 0x%x size\n", memsize);
		return;
	}

	for (i = 0; i < memsize/0x10; i++) {
		printf("[%04x]: %08x %08x %08x %08x\n", 
					i*0x10, data[4*i], data[4*i+1], data[4*i+2], data[4*i+3]);
	}

	return;
}

/* ******************************************
 * 函 数 名: lastwords_format_data
 * 功能描述: 临终遗言用户格式化数据空间
 * 输入参数: void
 * 输出参数: void	
 * 返 回 值: void
 * ****************************************** */
void lastwords_format_data(void)
{
	int ret = 0;
	
	ret = ioctl(lw_fd, LASTWORDS_FORMAT_MEM, 0);
	if (ret) 
		printf("fail to ioctl LASTWORDS_FORMAT_MEM cmd (%d)\n", errno);
	
}

/* ******************************************
 * 函 数 名: lastwords_main_control
 * 功能描述: 临终遗言用户解析程序主循环
 * 输入参数: void
 * 输出参数: void	
 * 返 回 值: void
 * ****************************************** */
void lastwords_main_control(void)
{
	char *pbuf = NULL;
	
	while (1) {
		printf("\n*************** Lastwords Main Control ***************\n");
		printf("1.Show Header Info\n");
		printf("2.Show All Info\n");
		printf("3.Show Mem Hex\n");
		printf("4.Format Data\n");
		printf("5.Trigger User Record\n");
		printf("0.Close\n");
		printf("please input choice:\n");

		pbuf = fgets(lw_tmp, sizeof(lw_tmp), stdin);
		if (NULL == pbuf) {
			continue;
		}

		switch (*pbuf) {
			/* Show Header Info */
			case '1':
				lastwords_show_header();
				break;
			/* Show All Info */
			case '2':
				lastwords_show_all();
				break;
			/* show mem hex */
			case '3': {
				unsigned int addr = 0;

				printf("please inupt start addr:");scanf("%d", &addr);				
				lastwords_show_memhex(addr);
				break;		
			}
			/* Format Data */
			case '4':
				lastwords_format_data();
				break;
			/* Trigger User Record */
			case '5': {
				char temp[128] = {0};
				int fd = -1;

				printf("please inupt trigger message:");
				(void)fgets(temp, sizeof(temp), stdin);

				fd = open("/proc/lastwords", O_RDWR);
				if (0 > fd) {
					printf("fail to open /proc/lastwords");
					break;
				}
					
				(void)write(fd, temp, strlen(temp)+1);
		
				close(fd);
				break;
			}	
			case '0':				
				return;
			default:
				printf("input error\n");
		}
	}
}


/* ******************************************
 * 函 数 名: main
 * 功能描述: 临终遗言用户解析程序
 * 输入参数: int argc
 			 void *argv
 * 输出参数: void	
 * 返 回 值: int
 * ****************************************** */
int main(int argc, void *argv)
{
	lw_fd = open("/dev/lastwords", O_RDWR);
	if (0 >= lw_fd) {
		printf("fail to open lastwords device\n");
		return 0;
	}
	
	lastwords_main_control();
	
	close(lw_fd);	
	return 0;
}




