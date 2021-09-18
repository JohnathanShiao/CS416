// Student name: Dustin Shiao
// Ilab machine used: perl

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


void floating_point_exception_handler(int signum) {

	printf("I am slain!\n");
	/* Do your tricks here */
	//used gdb to see the stack, find address of where program counter was stored
	//move pointer and increment program counter
	void* ptr = &signum;
	ptr = ptr+0xcc;
	*(int*) ptr+=0x3;
}

int main() {

	int x = 5, y = 0, z = 0;

	signal(SIGFPE, floating_point_exception_handler);

	z = x / y;
	
	printf("I live again!\n");

	return 0;
}
