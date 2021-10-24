// File:	mypthread_t.h

// List all group member's name: Dustin Shiao (ds1576), Henry Chu (hc798)
// username of iLab: pascal
// iLab Server:

#ifndef MYTHREAD_T_H
#define MYTHREAD_T_H

#define _GNU_SOURCE

/* To use Linux pthread Library in Benchmark, you have to comment the USE_MYTHREAD macro */
#define USE_MYTHREAD 1

/* include lib header files that you need here: */
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <signal.h>
#include <sys/time.h>
#include <string.h>
#include <stdatomic.h>

typedef uint mypthread_t;

typedef struct threadControlBlock {
	/* add important states in a thread control block */
	// thread Id
	// thread status
	// thread context
	// thread stack
	// thread priority
	// And more ...

	// YOUR CODE HERE
	mypthread_t t_id;
	//0 = ready
	//1 = running
	//2 = blocked
	int status;
	ucontext_t context;

	int time;
} tcb;

typedef struct blocked_queue {
	// mypthread_t t_id;
	tcb* threadControlBlock;
	struct blocked_queue* next;
} blocked_queue;

/* mutex struct definition */
typedef struct mypthread_mutex_t {
	/* add something here */

	int locked;
	mypthread_t t_id;
	blocked_queue* blockedQueueHead;

	// YOUR CODE HERE
} mypthread_mutex_t;

/* define your data structures here: */
// Feel free to add your own auxiliary data structures (linked list or queue etc...)

// YOUR CODE HERE

typedef struct run_queue {
	tcb* threadControlBlock;
	struct run_queue* next;
} run_queue;

typedef struct finished_queue {
	mypthread_t t_id;
	void* value;
	struct finished_queue* next;
} finished_queue;

/* Function Declarations: */

/* create a new thread */
int mypthread_create(mypthread_t * thread, pthread_attr_t * attr, void
    *(*function)(void*), void * arg);

/* give CPU pocession to other user level threads voluntarily */
int mypthread_yield();

/* terminate a thread */
void mypthread_exit(void *value_ptr);

/* wait for thread termination */
int mypthread_join(mypthread_t thread, void **value_ptr);

/* initial the mutex lock */
int mypthread_mutex_init(mypthread_mutex_t *mutex, const pthread_mutexattr_t
    *mutexattr);

/* aquire the mutex lock */
int mypthread_mutex_lock(mypthread_mutex_t *mutex);

/* release the mutex lock */
int mypthread_mutex_unlock(mypthread_mutex_t *mutex);

/* destroy the mutex */
int mypthread_mutex_destroy(mypthread_mutex_t *mutex);

void* myMalloc(int size);

static void sched_stcf();

static void sched_mlfq();

void signalHandler(int signum);

void enqueueSTCF(tcb* threadBlock);

tcb* dequeueSTCF();

static void schedule();

void resetMLFQ();

void enqueueMLFQ(tcb* threadBlock);

tcb* dequeueMLFQ();

#ifdef USE_MYTHREAD
#define pthread_t mypthread_t
#define pthread_mutex_t mypthread_mutex_t
#define pthread_create mypthread_create
#define pthread_exit mypthread_exit
#define pthread_join mypthread_join
#define pthread_mutex_init mypthread_mutex_init
#define pthread_mutex_lock mypthread_mutex_lock
#define pthread_mutex_unlock mypthread_mutex_unlock
#define pthread_mutex_destroy mypthread_mutex_destroy
#endif

#endif
