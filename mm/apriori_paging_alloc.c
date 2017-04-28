#include <linux/apriori_paging_alloc.h>
#include <linux/syscalls.h>

/*
 * !!_AprioriPaging_!!
 * This system has two functionalities depending on the value of the option variable
 *
 * if ( option > 0 )
 * It will take the process name as an argument and
 * every process which will be created with
 * this name will use our allocation mechanism
 *
 * if ( option < 0 )
 * It will take the pid as an argument and will force the process that has
 * this pid to use our allocation mechanism
 *
 */

/* ALL golbals here */
char apriori_paging_process[CONFIG_NR_CPUS][MAX_PROC_NAME_LEN];
ep_stats_t ep_statistics[CONFIG_NR_CPUS];
ep_stats_t zeroed_stats;

unsigned char enable_dump_stack;
unsigned char enable_prints;
unsigned char enable_stats;

ep_stats_t* indexof_process_stats(const char* proc_name);

inline unsigned long get_current_time(void) {
	unsigned long int time = 0;
	struct timespec tv;

	getnstimeofday(&tv);

	time = ((tv.tv_sec * BILLION) + tv.tv_nsec);
	return time;
}

inline void record_start_event(ep_stats_t *application, ep_event_t event) {
	if (application != NULL) {
		application->start_time = get_current_time();
		application->kernel_entry++;
		
		if (event < EP_MAX_EVENT) {
			application->counters[event]++;
		}
	}
}

inline void record_end_event(ep_stats_t *application, ep_event_t event) {
	if (application != NULL) {
		application->end_time = get_current_time();
		application->kernel_time += (application->end_time - application->start_time);
	}
}

inline void incr_pgfault_count(ep_stats_t *application) {
	if (application != NULL) {
		application->counters[EP_PGFAULT_EVENT]++;
	}
}

inline void incr_mmap_count(ep_stats_t *application) {
	if (application != NULL) {
		application->counters[EP_MMAP_EVENT]++;
	}
}

inline void incr_mremap_count(ep_stats_t *application) {
	if (application != NULL) {
		application->counters[EP_MREMAP_EVENT]++;
	}
}

int indexof_apriori_paged_process(const char* proc_name);

asmlinkage long sys_ep_control_syscall(int val) {

	pr_err("\n======================Inside system calls (%s)"
		"============================", __func__);

	switch (val) {
		case 1:
			enable_dump_stack = 1;
			pr_err("EP: Enabled stack dump");
			break;
		case -1:
			enable_dump_stack = 0;
			pr_err("EP: Disabled stack dump");
			break;

		case 2:
			enable_prints = 1;
			pr_err("EP: Enabled mmap prints");
			break;
		case -2:
			enable_prints = 0;
			pr_err("EP: disabled mmap prints");
			break;
		case 3:
			enable_stats = 1;
			pr_err("EP: Enabled stats");
			break;
		case -3:
			enable_stats = 0;
			pr_err("EP: Disabled stats");
			break;
	}
	return 0;
}

// asmlinkage long sys_list_ep_apps(void) {
asmlinkage long sys_list_ep_apps(int is_stats) {
	int i = 0;
	
	if (is_stats == FOR_EAGER_PAGING) {
		pr_err(" =================== Listing all Eager Paging Enabled"
			" Applications =========================\n");
		for ( i = 0 ; i < CONFIG_NR_CPUS ; i++ ) {
			if (apriori_paging_process[i][0] != '\0') {
				pr_err("%s\n", apriori_paging_process[i]);
			}
		}
	} else if (is_stats == FOR_STATISTICS) {
		pr_err(" =================== Listing all Statistics Enabled"
			" Applications =========================\n");
		for (i = 0; i < CONFIG_NR_CPUS; ++i) {
			if (ep_statistics[i].name[0] != '\0') {
				pr_err("%-20s | Kernel Time : %-11lu | Kernel Entry %-11lu |"
				" Page Fault Count: %-11lu | Mmap Count: %-11lu"
				"| Mremap Count: %-11lu\n",
				ep_statistics[i].name, ep_statistics[i].kernel_time,
				ep_statistics[i].kernel_entry,
				ep_statistics[i].counters[EP_PGFAULT_EVENT],
				ep_statistics[i].counters[EP_MMAP_EVENT],
				ep_statistics[i].counters[EP_MREMAP_EVENT]);
			}
		}
	}
	return 0xdeadbeef;
}

asmlinkage long sys_clear_ep_apps_list(const char __user* process_name, int
is_stats) {
	int i = 0;
	pr_err("\n======================Inside system calls (%s)"
		"============================", __func__);

	if (is_stats == FOR_EAGER_PAGING) {
		if (process_name == NULL) {
			for ( i = 0 ; i < CONFIG_NR_CPUS ; i++ ) {
				if (apriori_paging_process[i][0] != '\0') {
					apriori_paging_process[i][0] = '\0';
				}
			}
		} else {
			int ret = indexof_apriori_paged_process(process_name);
			if (ret != -1) {
				apriori_paging_process[ret][0] = '\0';
			} else {
				pr_err("Application name %s not found in the list\n", process_name);
			}
		}
	} else if (is_stats == FOR_STATISTICS) { /* syscall is to update stats information */
		if (process_name == NULL) {
			for ( i = 0 ; i < CONFIG_NR_CPUS ; i++ ) {
				if (ep_statistics[i].name[0] != '\0') {
					ep_statistics[i] = zeroed_stats;
				}
			}
		} else {
			ep_stats_t *ret = indexof_process_stats(process_name);
			if (ret != NULL) {
				*ret = zeroed_stats;
			} else {
				pr_err("Application name %s not found in the list\n", process_name);
			}
		}

	} else if (is_stats == 2) { /* reset the counters of stats (not remove) */
		if (process_name == NULL) {
			for ( i = 0 ; i < CONFIG_NR_CPUS ; i++ ) {
				if (ep_statistics[i].name[0] != '\0') {
					char temp[MAX_PROC_NAME_LEN];
					
					strncpy(temp, ep_statistics[i].name, MAX_PROC_NAME_LEN);
					
					ep_statistics[i] = zeroed_stats;
					
					strncpy(ep_statistics[i].name, temp, MAX_PROC_NAME_LEN);
				}
			}
		} else {
			ep_stats_t *ret = indexof_process_stats(process_name);
			if (ret != NULL) {
				char temp[MAX_PROC_NAME_LEN];
				strncpy(temp, ret->name, MAX_PROC_NAME_LEN);
				
				*ret = zeroed_stats;

				strncpy(ret->name, temp, MAX_PROC_NAME_LEN);

			} else {
				pr_err("Application name %s not found in the list\n", process_name);
			}
		}
	}

	if (is_stats > 1) {
		sys_list_ep_apps(1);
	} else {
		sys_list_ep_apps(0);
	}
	return 0xdeadbeef;
}


/* Register process for apriori paging or collecting statistics */
int ep_register_process(const char *proc_name, int option) {
	/*
	char proc[MAX_PROC_NAME_LEN];
    struct task_struct *tsk;
    unsigned long pid;
	*/
    unsigned int i = 0;
    unsigned long ret = 0;
	printk(KERN_ALERT "In function %s with proc_name = %s and option = %d\n", 
		__func__, proc_name, option);
	
	/* Enable collection of statistics for the process `proc_name` */
	if (option == FOR_STATISTICS) {
		i = 0;

		while(ep_statistics[i].name[0] != '\0') {
			i++;
		}

		ret = strncpy(ep_statistics[i].name, proc_name,
			MAX_PROC_NAME_LEN);
	} else if (option == FOR_EAGER_PAGING) {
		/* Enabled eager Paging for proc_name process */
		i = 0;
		while(apriori_paging_process[i][0] != '\0') {
			i++;
		}

		ret = strncpy(apriori_paging_process[i], proc_name,
			MAX_PROC_NAME_LEN);
    }

	/* Disabled eager paging through pids for now */
#if 0
	/* Using pid */
    if ( option < 0 ) {
        for ( i = 0 ; i < CONFIG_NR_CPUS ; i++ ) {
            if( i < num_procs ) {
                ret = kstrtoul(proc_name,10,&pid);
                if ( ret == 0 ) {
                    tsk = find_task_by_vpid(pid);
                    tsk->mm->apriori_paging_en = 1;
                }
            }
        }
    }
#endif 
	return 0xdeadbeef;
}



SYSCALL_DEFINE3(apriori_paging_alloc, const char __user**, proc_name, unsigned
int, num_procs, int, option)
{
    char proc[MAX_PROC_NAME_LEN];
	printk(KERN_ALERT "\n\n======================\nInside system calls (%s)"
		"\n============================\n\n", __func__);

	if (proc_name != NULL) {
		!strncpy_from_user(proc, proc_name[0], MAX_PROC_NAME_LEN);
		return ep_register_process(proc, option);
	}
    return 0;
}

/*
 * This function checks whether a process name provided matches from the list
 *
 */

int indexof_apriori_paged_process(const char* proc_name)
{
    unsigned int i;
    for ( i = 0 ; i < CONFIG_NR_CPUS ; i++ )     {
        if (!strncmp(proc_name, apriori_paging_process[i], MAX_PROC_NAME_LEN))
            return i;
    }

    return -1;
}

ep_stats_t* indexof_process_stats(const char* proc_name)
{
    unsigned int i;
    for ( i = 0 ; i < CONFIG_NR_CPUS ; i++ )     {
        if (!strncmp(proc_name, ep_statistics[i].name, MAX_PROC_NAME_LEN))
            return &ep_statistics[i];
    }
    return NULL;
}

int is_process_of_apriori_paging(const char* proc_name)
{
    unsigned int i;
    for ( i = 0 ; i < CONFIG_NR_CPUS ; i++ )     {
        if (!strncmp(proc_name, apriori_paging_process[i], MAX_PROC_NAME_LEN))
            return 1;
    }

    return 0;
}
