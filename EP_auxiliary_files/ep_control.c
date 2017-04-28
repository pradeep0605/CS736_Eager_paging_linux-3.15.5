#include <stdio.h>
#include <fcntl.h>
#include <linux/kernel.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#if 0

#define LIST_EP_APPS 321
#define LIST_STATS 321
#define CLEAR_EP_APPS_LIST 322
#define CLEAR_STATS_LIST 322
#define CONTROL_EP 323

#else

#define LIST_EP_APPS 320
#define LIST_STATS 320
#define CLEAR_EP_APPS_LIST 321
#define CLEAR_STATS_LIST 321
#define CONTROL_EP 322

#endif

#define __NR_apriori_paging_alloc 318
#define ADD_STATS 318

enum ep_register_type {FOR_EAGER_PAGING = 0,   
                       FOR_STATISTICS = 1}; 

int main(int argc, char **argv) {

	printf("%d, %d\n", FOR_EAGER_PAGING, FOR_STATISTICS);
	if (argc < 2 || (argc == 2 && 
	(strcmp(argv[1],"--help") == 0 || strcmp(argv[1], "help") == 0))) {
		printf("Usage: ./ep_control [--add | --list | --clear app_name |"
		" --clear-all | --ctrl #number | --add-stats | --clear-stats |\n"
		" --clear-all-stats | --reset-stats | --reset-all-stats | --list-stats ]\n");
		printf("--ctrl #number options: \n1) enabled stack dump (-1) for disabling ]\n");
		printf("2) enabled prints (-2) for disabling ]\n");
		printf("3) enabled stats (-3) disabling stats ]\n");
		return 0;
	}
	
	if (strcmp(argv[1], "--add") == 0) {
	 	if (argc == 3) {
			printf("Returned %d\n",
			syscall(__NR_apriori_paging_alloc, &argv[2], argc-2, FOR_EAGER_PAGING));
		} else {
			printf("Invalid number of arguments for %s\n", argv[1]);
		}

	} else if (strcmp(argv[1], "--list") == 0) {
		printf("Calling sys_list_ep_apps syscall. Returned %x\n",
			(unsigned int) syscall(LIST_EP_APPS, FOR_EAGER_PAGING));
	} else if (strcmp(argv[1], "--clear-all") == 0) {
		printf("Calling sys_clear_ep_list syscall. Returned %x\n",
			(unsigned int) syscall(CLEAR_EP_APPS_LIST, NULL, FOR_EAGER_PAGING));
	} else if (strcmp(argv[1], "--clear") == 0) {
	 	if (argc == 3) {
			printf("Calling sys_clear_ep_list syscall. Returned %x\n",
				(unsigned int) syscall(CLEAR_EP_APPS_LIST, argv[2], FOR_EAGER_PAGING));
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
			(unsigned int) syscall(ADD_STATS, &argv[2], 1, FOR_STATISTICS));
		} else {
			printf("Invalid number of arguments for %s\n", argv[1]);
		}
	} else if (strcmp(argv[1], "--clear-stats") == 0) {
	 	if (argc == 3) {
			printf("Returned %x\n",
			(unsigned int) syscall(CLEAR_STATS_LIST, argv[2], FOR_STATISTICS));
		} else {
			printf("Invalid number of arguments for %s\n", argv[1]);
		}
	} else if (strcmp(argv[1], "--clear-all-stats") == 0) {
	 	if (argc == 2) {
			printf("Returned %x\n",
			(unsigned int) syscall(CLEAR_STATS_LIST, NULL, FOR_STATISTICS));
		} else {
			printf("Invalid number of arguments for %s\n", argv[1]);
		}
	} else if (strcmp(argv[1], "--reset-stats") == 0) {
		if (argc == 3) {
			printf("Returned %x\n",
			(unsigned int) syscall(CLEAR_STATS_LIST, argv[2],
				FOR_STATISTICS + 1));
		} else {
			printf("Invalid number of arguments for %s\n", argv[1]);
		}
	} else if (strcmp(argv[1], "--reset-all-stats") == 0) {
		if (argc == 2) {
			printf("Returned %x\n",
			(unsigned int) syscall(CLEAR_STATS_LIST, NULL,
				FOR_STATISTICS + 1));
		} else {
			printf("Invalid number of arguments for %s\n", argv[1]);
		}
	} else if (strcmp(argv[1], "--list-stats") == 0) {
	 	if (argc == 2) {
			printf("Returned %x\n",
			(unsigned int) syscall(LIST_STATS, FOR_STATISTICS));
		} else {
			printf("Invalid number of arguments for %s\n", argv[1]);
		}
	} else {
		printf("Invalid argument %s\n", argv[1]);
	}
	
	return 0;
}
