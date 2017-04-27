#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>

int main() {
        int *a, i;
        char * message = "Memory Allocation Happening here \n\n\n\n\n";
	int n =  1024  * 4 * 32 ; 
	write(1, message, strlen(message));
	a = malloc(n);
	
        for(i = 0; i< n / sizeof(int); i++)
                a[i] = 0;
        return 0;
}
