#include <sys/syscall.h>
#include <stdio.h>
#include <stdlib.h>
int main(int argc, char **argv) {
	
	if (argc < 2) {
		printf("improper usage: lab1_test num1 num2\n");
		exit(0);
	}

	int a = atoi(argv[1]);
	int b = atoi(argv[2]);
	int c;

	int x = syscall(319,a,b,&c);
	printf("return: %d\nsum: %d\n",x,c);
}
