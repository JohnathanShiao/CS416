#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>

#define COUNTER_VALUE (1UL << 24)
#define NUMCPUS 1000
#define THRESHOLD 1000

typedef struct __counter_t {
	long global; // global count
	pthread_mutex_t glock; // global lock
	int local[NUMCPUS]; // per-CPU count
	pthread_mutex_t llock[NUMCPUS]; // ... and locks
} counter_t;

counter_t *c;

void init() {
	c->global = 0;

	if (pthread_mutex_init(&c->glock, NULL) != 0)
	{
		printf("Mutex init has failed\n");
		exit(1);
	}

	for (int i = 0; i < NUMCPUS; ++i)
	{
		c->local[i] = 0;
		if (pthread_mutex_init(&c->llock[i], NULL) != 0)
		{
        	printf("Mutex init has failed\n");
        	exit(1);
		}
	}
}

void update() {
	pthread_t t_id = pthread_self();
	int cpu = t_id % NUMCPUS;
	pthread_mutex_lock(&c->llock[cpu]);

	c->local[cpu] += 1;

	if (c->local[cpu] >= THRESHOLD)
	{
		// transfer to global (assumes amt>0)
		pthread_mutex_lock(&c->glock);
		c->global += c->local[cpu];
		// printf("%ld\n",c->global);
		pthread_mutex_unlock(&c->glock);
		c->local[cpu] = 0;
	}

	pthread_mutex_unlock(&c->llock[cpu]);
}

long get() {
	pthread_mutex_lock(&c->glock);
	long val = c->global;
	pthread_mutex_unlock(&c->glock);
	return val; // only approximate!
}

void* count()
{
	// counter_t *c = p;

    for (int i = 0; i < COUNTER_VALUE; ++i)
    {
        update();
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

	c = malloc(sizeof(counter_t));
	init();
    pthread_t pthreadArray[numThreads];

	struct timeval start, end, result;
    gettimeofday(&start, NULL);

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

	long counter = get();

    double total_time = (result.tv_sec * 1000000) + result.tv_usec;

    printf("Counter finish in %f microseconds\n", total_time);
    printf("The value of counter should be %ld\n", COUNTER_VALUE * numThreads);
    printf("The value of counter is %ld\n", counter);

	return 0;
}
