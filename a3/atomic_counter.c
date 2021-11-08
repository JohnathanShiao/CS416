#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>

#define COUNTER_VALUE (1UL << 24)
long counter = 0;
pthread_mutex_t lock;

void* count()
{
    for (int i = 0; i < COUNTER_VALUE; ++i)
    {
        __atomic_fetch_add(&counter, 1, __ATOMIC_SEQ_CST);
    }
}

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        printf("Invalid number of arguments");
        exit(1);
    }

    int numThreads = atoi(argv[1]);
    pthread_t pthreadArray[numThreads];

    struct timeval start, end, result;
    gettimeofday(&start,NULL);

    for (int i = 0; i < numThreads; ++i)
    {
        pthread_t t_id;
        pthread_create(&t_id, NULL, &count, NULL);
        pthreadArray[i] = t_id;
    }

    for (int i = 0; i < numThreads; ++i)
    {
        pthread_join(pthreadArray[i], NULL);
    }

    gettimeofday(&end, NULL);
    timersub(&end, &start, &result);

    double total_time = (result.tv_sec * 1000000) + result.tv_usec;

    printf("Counter finish in %f microseconds\n", total_time);
    printf("The value of counter should be %ld\n", COUNTER_VALUE * numThreads);
    printf("The value of counter is %ld\n", counter);

	return 0;
}
