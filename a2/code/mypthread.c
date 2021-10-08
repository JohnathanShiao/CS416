// File:	mypthread.c

// List all group member's name: Dustin Shiao, Henry Chu
// username of iLab: pascal
// iLab Server: 

#include "mypthread.h"
#include <ucontext.h>
#include <signal.h>
// INITAILIZE ALL YOUR VARIABLES HERE
// YOUR CODE HERE
tcb* currentThread=NULL;
tcb* schedulerContext=NULL;
void runner(void*(*function)(void*), void* arg);

int t_idcounter = 0;

void* myMalloc(int size)
{
	void* temp = calloc(1,size);
	if(temp==NULL)
	{
		printf("Fatal Error, malloc has returned null");
		exit(1);
	}
	return temp;
}

/* create a new thread */
int mypthread_create(mypthread_t * thread, pthread_attr_t * attr,
                      void *(*function)(void*), void * arg) {
       	// create Thread Control Block
       	// create and initialize the context of this thread
       	// allocate space of stack for this thread to run
       	// after everything is all set, push this thread int
       	// YOUR CODE HERE
		//TCB
		tcb* newThread = myMalloc(sizeof(tcb));
		newThread->t_id = t_idcounter++;
		newThread->status = 0;
		//Thread Context
		ucontext_t current;
		if(getcontext(&current)==-1)
		{
			printf("Error, getcontext had an error\n");
			exit(1);
		}
		void* stack = myMalloc(SIGSTKSZ);
		current.uc_link=NULL;
		current.uc_stack.ss_sp=stack;
		current.uc_stack.ss_size=SIGSTKSZ;
		current.uc_stack.ss_flags=0;
		makecontext(&current,&runner,2,function,arg);
		newThread->context = current;
		//Runqueue

    return 0;
};

/* give CPU possession to other user-level threads voluntarily */
int mypthread_yield() {

	// change thread state from Running to Ready
	// save context of this thread to its thread control block
	// wwitch from thread context to scheduler context

	// YOUR CODE HERE
	swapcontext(&(currentThread->context),&schedulerContext);
	return 0;
};

/* terminate a thread */
void mypthread_exit(void *value_ptr) {
	// Deallocated any dynamic memory created when starting this thread

	// YOUR CODE HERE
};


/* Wait for thread termination */
int mypthread_join(mypthread_t thread, void **value_ptr) {

	// wait for a specific thread to terminate
	// de-allocate any dynamic memory created by the joining thread

	// YOUR CODE HERE
	return 0;
};

/* initialize the mutex lock */
int mypthread_mutex_init(mypthread_mutex_t *mutex,
                          const pthread_mutexattr_t *mutexattr) {
	//initialize data structures for this mutex

	// YOUR CODE HERE
	return 0;
};

/* aquire the mutex lock */
int mypthread_mutex_lock(mypthread_mutex_t *mutex) {
        // use the built-in test-and-set atomic function to test the mutex
        // if the mutex is acquired successfully, enter the critical section
        // if acquiring mutex fails, push current thread into block list and //
        // context switch to the scheduler thread

        // YOUR CODE HERE
        return 0;
};

/* release the mutex lock */
int mypthread_mutex_unlock(mypthread_mutex_t *mutex) {
	// Release mutex and make it available again.
	// Put threads in block list to run queue
	// so that they could compete for mutex later.

	// YOUR CODE HERE
	return 0;
};


/* destroy the mutex */
int mypthread_mutex_destroy(mypthread_mutex_t *mutex) {
	// Deallocate dynamic memory created in mypthread_mutex_init

	return 0;
};

/* scheduler */
static void schedule() {
	// Every time when timer interrup happens, your thread library
	// should be contexted switched from thread context to this
	// schedule function

	// Invoke different actual scheduling algorithms
	// according to policy (STCF or MLFQ)

	// if (sched == STCF)
	//		sched_stcf();
	// else if (sched == MLFQ)
	// 		sched_mlfq();

	// YOUR CODE HERE

// schedule policy
#ifndef MLFQ
	// Choose STCF
#else
	// Choose MLFQ
#endif

}

/* Preemptive SJF (STCF) scheduling algorithm */
static void sched_stcf() {
	// Your own implementation of STCF
	// (feel free to modify arguments and return types)

	// YOUR CODE HERE
}

/* Preemptive MLFQ scheduling algorithm */
static void sched_mlfq() {
	// Your own implementation of MLFQ
	// (feel free to modify arguments and return types)

	// YOUR CODE HERE
}

// Feel free to add any other functions you need

// YOUR CODE HERE
void runner(void*(*function)(void*), void* arg){
	function(arg);
}