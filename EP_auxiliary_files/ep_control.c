#include <stdio.h>
#include <fcntl.h>
#include <linux/kernel.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define LIST_EP_APPS 321
#define LIST_STATS 321

#define CLEAR_EP_APPS_LIST 322
#define CLEAR_STATS_LIST 322

#define CONTROL_EP 323

#define __NR_apriori_paging_alloc 318
#define ADD_STATS 318

int main(int argc, char **argv) {

	if (argc < 2) {
		printf("Usage: ./ep_control [--add | --list | --clear app_name |"
		" --clear-all | --ctrl #number | --add-stats | --clear-stats |"
		" --clear-all-stats | --list-stats ]\n");
		printf("--ctrl #number options: \n1) enabled stack dump (-1) for disabling ]\n");
		printf("2) enabled prints (-2) for disabling ]\n");
		printf("3) enabled stats (-3) disabling stats ]\n");
		return 0;
	}
	
	if (strcmp(argv[1], "--add") == 0) {
	 	if (argc == 3) {
			syscall(__NR_apriori_paging_alloc, &argv[2], argc-2, 0);
		} else {
			printf("Invalid number of arguments for %s\n", argv[1]);
		}

	} else if (strcmp(argv[1], "--list") == 0) {
		printf("Calling sys_list_ep_apps syscall. Returned %x\n",
			(unsigned int) syscall(LIST_EP_APPS, 0));
	} else if (strcmp(argv[1], "--clear-all") == 0) {
		printf("Calling sys_clear_ep_list syscall. Returned %x\n",
			(unsigned int) syscall(CLEAR_EP_APPS_LIST, NULL, 0));
	} else if (strcmp(argv[1], "--clear") == 0) {
	 	if (argc == 3) {
			printf("Calling sys_clear_ep_list syscall. Returned %x\n",
				(unsigned int) syscall(CLEAR_EP_APPS_LIST, argv[2], 0));
		} else {
			printf("Invalid number of arguments for %s\n", argv[1]);
		}
	} else if (strcmp(argv[1], "--ctrl") == 0) {
		if (argc == 3) {
			int cmd = atoi(argv[2]);
			printf("Calling sys_ep_control_syscall syscall. Returned %x\n",
				(unsigned int) syscall(CONTROL_EP, cmd));

		} else {
			printf("Invalid number of arguments for %s\n", argv[1]);
		}
	} else if (strcmp(argv[1], "--add-stats") == 0) {
	 	if (argc == 3) {
			printf("Returned %x\n",
			(unsigned int) syscall(ADD_STATS, &argv[2], 1, 1) );
		} else {
			printf("Invalid number of arguments for %s\n", argv[1]);
		}
	} else if (strcmp(argv[1], "--clear-stats") == 0) {
	 	if (argc == 3) {
			printf("Returned %x\n",
			(unsigned int) syscall(CLEAR_STATS_LIST, argv[2], 1));
		} else {
			printf("Invalid number of arguments for %s\n", argv[1]);
		}
	} else if (strcmp(argv[1], "--clear-all-stats") == 0) {
	 	if (argc == 2) {
			printf("Returned %x\n",
			(unsigned int) syscall(CLEAR_STATS_LIST, NULL, 1));
		} else {
			printf("Invalid number of arguments for %s\n", argv[1]);
		}
	} else if (strcmp(argv[1], "--list-stats") == 0) {
	 	if (argc == 2) {
			printf("Returned %x\n",
			(unsigned int) syscall(LIST_STATS, 1));
		} else {
			printf("Invalid number of arguments for %s\n", argv[1]);
		}
	} else {
		printf("Invalid argument %s\n", argv[1]);
	}
	
	return 0;
}
