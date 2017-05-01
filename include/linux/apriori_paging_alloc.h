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
#include <linux/mmzone.h>
//#include <linux/timekeeping.h>

#define BILLION (1000000000)
#define MILLION (1000000)
#define THOUSANDS (1000)

/* This is a hard coded number. For the Fedora-16 running in virtual machine.
 * This number indicates the time neded to do a context switch from user space
 * to kernel space and back.
 */ 
#define CTXT_SWTCH_TIME (1500)

#define EP_MAX_ORDER 21
enum ep_register_type {FOR_EAGER_PAGING = 0,
					   FOR_STATISTICS = 1};

typedef enum event_type { EP_MMAP_EVENT,
  						    EP_MREMAP_EVENT, 
							EP_PGFAULT_EVENT,
							EP_PGFAULT_MINOR_EVENT,
							EP_PGFAULT_MAJOR_EVENT,
							EP_ALLOC_ORDER_EVENT,
							EP_ALLOC_REQ_FROM_USR_SPACE,
							EP_MAX_EVENT
}  ep_event_t;


typedef struct eager_paging_statistics {
	char name[MAX_PROC_NAME_LEN]; /* Name of the process */
	unsigned long start_time;  /* Amount of time spent in kernel for allocation */
	unsigned long end_time;  /* Amount of time spent in kernel for allocation */
	unsigned long allocation_time; /* Amount of time spent for allocation */
	unsigned long kernel_time;  /* Total Amount of time spent in kernel
								for allocation and handling page faults */
	unsigned long kernel_entry; /* Number of time we entered kernel for allocation 
					kernel_entry = mmap_entry_count + pagefault_entry_count */
	
	/* Coutners for all the events */
	unsigned long counters[EP_MAX_EVENT];
	unsigned long timers[EP_MAX_EVENT];
	unsigned long orders[EP_MAX_ORDER + 1];
	unsigned long user_mem_req;
} ep_stats_t;


extern char apriori_paging_process[CONFIG_NR_CPUS][MAX_PROC_NAME_LEN];

extern unsigned char enable_dump_stack;
extern unsigned char enable_prints;
extern unsigned char enable_stats;
extern unsigned char enable_alloc_overhead_stats;

int is_process_of_apriori_paging(const char* proc_name);
inline unsigned long get_current_time(void);
inline void record_start_event(ep_stats_t *application, ep_event_t event);
inline void record_end_event(ep_stats_t *application, ep_event_t event);
inline void incr_pgfault_count(ep_stats_t *application);
inline void incr_mmap_count(ep_stats_t *application);
int ep_register_process(const char *proc_name, int option);
ep_stats_t* indexof_process_stats(const char* proc_name);
inline void record_alloc_event(ep_stats_t *application, ep_event_t event,
	unsigned long order);
inline void dec_event_counter(ep_stats_t *application, ep_event_t event);

#define ep_print(...) if (enable_prints) \
							pr_err(__VA_ARGS__)


#define ep_dump_stack() if (enable_dump_stack) \
							dump_stack()

/* All three macros assume that time is give in nano seconds */
#define IN_SECONDS(time)  ((double(time)) / (double(BILLION)))

#define IN_MICROSECS(time)  ((double(time)) / (double(MILLION)))

#define IN_MICROSECS(time)  ((double(time)) / (double(MILLION)))


#endif
