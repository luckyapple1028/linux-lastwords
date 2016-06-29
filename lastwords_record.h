
#ifndef	__LASTWORDS_RECORD__
#define __LASTWORDS_RECORD__

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

#define LAST_WORDS_ATTR_MAX (__LAST_WORDS_ATTR_MAX - LAST_WORDS_ATTR_UNSPEC - 1)

/* 记录内容类型 */
enum {
	LAST_WORDS_RECD_UNSPEC = 100,
	LAST_WORDS_RECD_TRIGGER,		/* 用户信息 */
	LAST_WORDS_RECD_TIME,			/* 系统时间 */
	LAST_WORDS_RECD_SYSINFO,		/* 系统信息 */
	LAST_WORDS_RECD_DMESG,			/* dmesg信息 */
	LAST_WORDS_RECD_PS,				/* ps进程信息 */
	LAST_WORDS_RECD_BACKTRACE,		/* back trace信息 */
	__LAST_WORDS_RECD_MAX,
};

#define LAST_WORDS_RECD_MAX (__LAST_WORDS_RECD_MAX - LAST_WORDS_RECD_UNSPEC - 1)


struct lws_head {
	__u16 lh_magic;				/* 魔数 */
	__u16 lh_ecc;				/* ECC校验码 */
	__u32 lh_len;				/* 记录信息长度(包括消息头) */
	__u32 reserved;				/* 保留备用 */
} __attribute__((aligned(4)));

struct lws_attr {
	__u16 type;					/* 记录数据类型 */
	__u16 len;					/* 记录数据长度(包括属性头) */
} __attribute__((aligned(4)));


struct lws_attr * lastwords_prepare_attr(__u16 attr_type);
int lastwords_print(const char *fmt, ...);
void lastwords_end_attr(struct lws_attr *plwa);
__u32 lastwords_dump_record(char *buf, __u32 len, __u32 off);
__u32 lastwords_get_recordlen(void);
void lastwords_format_record(void);

#endif

