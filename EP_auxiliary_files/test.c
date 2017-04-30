#include<stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>

int main()
{
#if 1
	unsigned long n = 1024 * 4;
	int k = 5;
	int * ptr = NULL;
	/*
	int *ptr = mmap(NULL, n, PROT_READ | PROT_WRITE, MAP_PRIVATE |
	MAP_ANONYMOUS, -1, 0);
	*/
	
	while (k--) {
		printf("Allocating %lf MB\n", (double)(n) / 1024 / 1024);
		write(1, "After this\n", strlen("After this\n"));
		// scanf("%d", &n);
		n = n * 10;
		ptr = malloc(n);
		
		printf("Returned address %p\n", ptr);
		int i = 0;
		for(i = 0; i < n / sizeof(int); ++i) {
			// ptr[i] = i;
		}
		/*
		for(i = n / sizeof(int) - 1; i >= 0 ; --i) {
			ptr[i] = i;
		}
		*/
	}
	getchar();
#endif
	return 0;
}
