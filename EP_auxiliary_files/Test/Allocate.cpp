#include<iostream>
#include<unistd.h>
#include<string.h>
#include<stdio.h>
using namespace std;

int main() {
	write(1, "Im here!\n\n\n",strlen("Im here!\n\n\n"));
	fflush(stdin);       
	int n = 0;
	// cin >> n;
	int * foo;
        foo = new int [1024 * 32 + 100]();
	write(1, "after",strlen("after"));
        return 0;
}
