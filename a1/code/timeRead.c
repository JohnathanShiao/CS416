#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
// Place any necessary global variables here
#define FILESIZE 512000000
int main(int argc, char *argv[]){

	/*
	 *
	 * Implement Here
	 *
	 *
	 */
	//create file called temp with read write
	int fd = open("temp", O_CREAT | O_RDONLY, 0777);
	//create another file descriptor just for writing
	int wfd = open("temp", O_WRONLY);
	if(fd<0)
	{
		printf("Fatal Error, could not create file named 'temp' in this directory.\n");
		close(wfd);
		close(fd);
		return 1;
	}
	//set writing file descriptor to end
	int result = lseek(wfd, FILESIZE-1,SEEK_SET);
	if(result <0)
	{
		printf("Error, lseek() was unable to stretch the file to 512MB.\n");
		close(wfd);
		close(fd);
		return 1;
	}
	//write
	result = write(wfd,"a",1);
	if(result<0)
	{
		printf("Error writing to file 'temp'.\n");
		close(wfd);
		close(fd);
		return 1;
	}
	int count = 0;
	//initialize buffer of 4KB
	char* buffer = calloc(1,4000);
	struct timeval start,end,diff;
	gettimeofday(&start,NULL);
	while(read(fd,buffer,4000)>0)
	{
		//counter to ensure syscalls actually take place
		count++;
	}
	//end timer
	gettimeofday(&end,NULL);
	free(buffer);
	//find difference
	timersub(&end,&start,&diff);
	//convert seconds into microseconds if needed
	double total_time = (diff.tv_sec*1000000)+diff.tv_usec;
	double avg_time = total_time/count;
	printf("Syscalls Performed: %d\n", count);
	printf("Total Elapsed Time: %f microseconds\n",total_time);
	printf("Average Time Per Syscall: %f microseconds\n", avg_time);
	remove("temp");
	close(wfd);
	close(fd);
	return 0;

}
