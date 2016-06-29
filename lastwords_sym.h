#ifndef	__LASTWORDS_SYM__
#define __LASTWORDS_SYM__




extern int *lw_nr_threads;				/* _nr_threads */	


typedef struct timespec (* lw__current_kernel_time_fun)(void);			/* __current_kernel_time */
extern lw__current_kernel_time_fun lw__current_kernel_time;


#endif /* __LASTWORDS_SYM__ */

