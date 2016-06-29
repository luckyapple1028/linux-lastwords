
#ifndef	__LASTWORDS_EXPORT__
#define __LASTWORDS_EXPORT__


#define LASTWORDS_BIT(n)				(1 << n)
#define LASTWORDS_EXPORT_TIME			LASTWORDS_BIT(0)		/* 系统时间 */
#define LASTWORDS_EXPORT_SYSINFO		LASTWORDS_BIT(1)		/* 系统信息 */
#define LASTWORDS_EXPORT_DMESG			LASTWORDS_BIT(2)		/* dmesg信息 */
#define LASTWORDS_EXPORT_PS 			LASTWORDS_BIT(3)		/* ps进程信息 */
#define LASTWORDS_EXPORT_BACKTRACE		LASTWORDS_BIT(4)		/* backtrace信息 */

int lastwords_export_attr(__u16 attr_type, __u32 export_type, char *trigger);
void lastwords_export_clear(void);
__u32 lastwords_export_dump(char *buf, __u32 len, __u32 off);

#endif

