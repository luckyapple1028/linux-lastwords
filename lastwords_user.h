
#ifndef	__LASTWORDS_USER__
#define __LASTWORDS_USER__

/* 记录触发类型 */
enum {
	LAST_WORDS_ATTR_UNSPEC = 0,
	LAST_WORDS_ATTR_USER,			/* 用户强制触发 */	
	LAST_WORDS_ATTR_NETADR,			/* 网络设备地址改变 */
	LAST_WORDS_ATTR_MODULE,			/* 内核加载卸载模块 */
	LAST_WORDS_ATTR_REBOOT,			/* 设备指令性重启(reboot) */
	LAST_WORDS_ATTR_PANIC,			/* 内核panic(kernel panic) */
	LAST_WORDS_ATTR_DIE,			/* 内核die(kernel die) */
	LAST_WORDS_ATTR_OOM,			/* 内核oom(kernel oom) */
	__LAST_WORDS_ATTR_MAX,
};

#define LAST_WORDS_ATTR_MAX 		(__LAST_WORDS_ATTR_MAX - LAST_WORDS_ATTR_UNSPEC - 1)
#define LAST_WORDS_ATTR_N(num)		((num) - LAST_WORDS_ATTR_UNSPEC - 1)

/* 记录内容类型 */
enum {
	LAST_WORDS_RECD_UNSPEC = 100,
	LAST_WORDS_RECD_TRIGGER,		/* 触发信息 */
	LAST_WORDS_RECD_TIME,			/* 系统时间 */
	LAST_WORDS_RECD_SYSINFO,		/* 系统信息 */
	LAST_WORDS_RECD_DMESG,			/* dmesg信息 */
	LAST_WORDS_RECD_PS,				/* ps进程信息 */
	LAST_WORDS_RECD_BACKTRACE,		/* back trace信息 */
	__LAST_WORDS_RECD_MAX,
};

#define LAST_WORDS_RECD_MAX			(__LAST_WORDS_RECD_MAX - LAST_WORDS_RECD_UNSPEC - 1)
#define LAST_WORDS_RECD_N(num)		((num) - LAST_WORDS_RECD_UNSPEC - 1)

struct lws_head {
	unsigned short lh_magic;			/* 魔数 */
	unsigned short lh_ecc;				/* ECC校验码 */
	unsigned int lh_len;				/* 记录信息长度(包括消息头) */
	unsigned int reserved;				/* 保留备用 */
} __attribute__((aligned(4)));

struct lws_attr {
	unsigned short type;				/* 记录数据类型 */
	unsigned short len;					/* 记录数据长度(包括属性头) */
} __attribute__((aligned(4)));


#define LASTWORDS_MAGIC			'L'

/* 读取头部信息 */
#define LASTWORDS_READ_HEAD			_IOR(LASTWORDS_MAGIC, 0x00, struct lws_head)
/* 获取内存空间大小 */
#define LASTWORDS_GET_MEMSIZE		_IOR(LASTWORDS_MAGIC, 0x01, unsigned int)
/* 格式化内存空间 */
#define LASTWORDS_FORMAT_MEM		_IO(LASTWORDS_MAGIC, 0x02)
/* 强制用户触发记录 */
#define LASTWORDS_TRIGGER_RECORD	_IOW(LASTWORDS_MAGIC, 0x03, char *)

#endif

