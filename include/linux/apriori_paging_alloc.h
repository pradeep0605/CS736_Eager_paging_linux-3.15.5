#ifndef __APRIORI_PAGING_ALLOC_H__
#define __APRIORI_PAGING_ALLOC_H__

// #define ENABLE_EP_MSG 1

#ifdef ENABLE_EP_MSG
#define ep_msg(...) pr_err(__VA_ARGS__)
#else
#define ep_msg(...)
#endif /* ENABLE_EP_MSG */

#define MAX_PROC_NAME_LEN 50

#include <linux/time.h>
//#include <linux/timekeeping.h>

#define BILLION (1000000000)
#define MILLION (1000000)
#define THOUSANDS (1000)

typedef struct eager_paging_statistics {
	char process_name[MAX_PROC_NAME_LEN]; /* Name of the process */
	unsigned long start_time;  /* Amount of time spent in kernel for allocation */
	unsigned long end_time;  /* Amount of time spent in kernel for allocation */
	unsigned long kernel_time;  /* Amount of time spent in kernel for allocation */
	unsigned long mmap_entry_count; /* Number of times entered mmap */
	unsigned long pagefault_entry_count; /* number of times entered page fault
	routine */

	unsigned long kernel_entry; /* Number of time we entered kernel for allocation 
					kernel_entry = mmap_entry_count + pagefault_entry_count */

} ep_stats_t;


extern char apriori_paging_process[CONFIG_NR_CPUS][MAX_PROC_NAME_LEN];

extern unsigned char enable_dump_stack;
extern unsigned char enable_prints;
extern unsigned char enable_stats;

int is_process_of_apriori_paging(const char* proc_name);
inline unsigned long get_current_time(void);
inline void record_start_event(ep_stats_t *application);
inline void record_end_event(ep_stats_t *application);
inline void incr_pgfault_count(ep_stats_t *application);
inline void incr_mmap_count(ep_stats_t *application);

ep_stats_t* indexof_process_stats(const char* proc_name);

#define ep_print(...) if (enable_prints) \
							pr_err(__VA_ARGS__)


#define ep_dump_stack() if (enable_dump_stack) \
							dump_stack()

/* All three macros assume that time is give in nano seconds */
#define IN_SECONDS(time)  ((double(time)) / (double(BILLION)))

#define IN_MICROSECS(time)  ((double(time)) / (double(MILLION)))

#define IN_MICROSECS(time)  ((double(time)) / (double(MILLION)))


#endif
