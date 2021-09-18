//Tested on iLab: perl
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
// Place any necessary global variables here

#define CALLS 250000

int main(int argc, char *argv[]){
	/*
	 *
	 * Implement Here
	 *
	 *
	 */
	struct timeval start, end, result;
	//counter to make sure calls are working
	int count = 0;
	//start timer
	gettimeofday(&start,NULL);
	for(int i = 0; i<CALLS; i++)
	{
		pid_t getpid();
		count++;
	}
	//end timer
	gettimeofday(&end,NULL);
	//find difference
	timersub(&end,&start,&result);
	//convert seconds into microseconds if needed
	double total_time = (result.tv_sec*1000000)+result.tv_usec;
	double avg_time = total_time/CALLS;
	printf("Syscalls Performed: %d\n", count);
	printf("Total Elapsed Time: %f microseconds\n",total_time);
	printf("Average Time Per Syscall: %f microseconds\n", avg_time);
	return 0;

}
