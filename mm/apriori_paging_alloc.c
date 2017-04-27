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

inline void record_start_event(ep_stats_t *application) {
	if (application != NULL) {
		application->start_time = get_current_time();
		application->kernel_entry++;
	}
}

inline void record_end_event(ep_stats_t *application) {
	if (application != NULL) {
		application->end_time = get_current_time();
		application->kernel_time += (application->end_time - application->start_time);
	}
}

inline void incr_pgfault_count(ep_stats_t *application) {
	if (application != NULL) {
		application->pagefault_entry_count++;
	}
}

inline void incr_mmap_count(ep_stats_t *application) {
	if (application != NULL) {
		application->mmap_entry_count++;
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
	
	if (is_stats == 0) {
		pr_err(" =================== Listing all Eager Paging Enabled"
			" Applications =========================\n");
		for ( i = 0 ; i < CONFIG_NR_CPUS ; i++ ) {
			if (apriori_paging_process[i][0] != '\0') {
				pr_err("%s\n", apriori_paging_process[i]);
			}
		}
	} else if (is_stats == 1) {
		pr_err(" =================== Listing all Statistics Enabled"
			" Applications =========================\n");
		for (i = 0; i < CONFIG_NR_CPUS; ++i) {
			if (ep_statistics[i].process_name[0] != '\0') {
				pr_err("%-20s | Kernel Time : %-11lu | Kernel Entry %-11lu |"
				" Page Fault Count: %-11lu | Mmap Count: %-11lu\n" ,
				ep_statistics[i].process_name, ep_statistics[i].kernel_time,
				ep_statistics[i].kernel_entry,
				ep_statistics[i].pagefault_entry_count,
				ep_statistics[i].mmap_entry_count);
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

	if (is_stats == 0) {
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
	} else if (is_stats == 1) { /* syscall is to update stats information */
		if (process_name == NULL) {
			for ( i = 0 ; i < CONFIG_NR_CPUS ; i++ ) {
				if (ep_statistics[i].process_name[0] != '\0') {
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

	}
	
	return 0xdeadbeef;
}


SYSCALL_DEFINE3(apriori_paging_alloc, const char __user**, proc_name, unsigned
int, num_procs, int, option)
{
    unsigned int i = 0;
    // char proc[MAX_PROC_NAME_LEN];
    unsigned long ret = 0;

    struct task_struct *tsk;
    unsigned long pid;

	
	printk(KERN_ALERT "\n\n======================\nInside system calls (%s)"
		"\n============================\n\n", __func__);


	/* Add the proc_name to the list of processes to be tracked for statistics
	 * */
	if (option == 1) {
		i = 0;
		if (num_procs == 1) {
			while(ep_statistics[i].process_name[0] != '\0') {
				i++;
			}

			ret = strncpy_from_user(ep_statistics[i].process_name, proc_name[0],
				MAX_PROC_NAME_LEN);
		}
	}

	/* Enabled eager Paging for proc_name process */
    if ( option == 0 ) {
		i = 0;
		if (num_procs == 1) {
			while(apriori_paging_process[i][0] != '\0') {
				i++;
			}
			
			ret = strncpy_from_user(apriori_paging_process[i], proc_name[0],
				MAX_PROC_NAME_LEN);
		}
    }

	/* Using pid */
    if ( option < 0 ) {
        for ( i = 0 ; i < CONFIG_NR_CPUS ; i++ ) {
            if( i < num_procs ) {
                ret = kstrtoul(proc_name[i],10,&pid);
                if ( ret == 0 ) {
                    tsk = find_task_by_vpid(pid);
                    tsk->mm->apriori_paging_en = 1;
                }
            }
        }
    }

    return ret;
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
        if (!strncmp(proc_name, ep_statistics[i].process_name, MAX_PROC_NAME_LEN))
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
