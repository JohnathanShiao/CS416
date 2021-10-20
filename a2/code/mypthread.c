// File:	mypthread.c

// List all group member's name: Dustin Shiao, Henry Chu
// username of iLab: pascal
// iLab Server: 

#include "mypthread.h"
#include <ucontext.h>
#include <signal.h>
// INITAILIZE ALL YOUR VARIABLES HERE
// YOUR CODE HERE
tcb* currentThread = NULL;
tcb* schedulerContext = NULL;
void runner(void*(*function)(void*), void* arg);

run_queue* runQueueHead = NULL;
blocked_queue* blockedQueueHead = NULL;
finished_queue* finishedQueueHead = NULL;

//how to tell if current thread is done
int done = 0;

int t_idcounter = 0;

void* myMalloc(int size)
{
	void* temp = calloc(1, size);

	if (temp == NULL)
	{
		printf("Fatal Error, malloc has returned null");
		exit(1);
	}

	return temp;
}

/* create a new thread */
int mypthread_create(mypthread_t* thread, pthread_attr_t* attr, void *(*function)(void*), void* arg)
{
	
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

	if (getcontext(&current) == -1)
	{
		printf("Error, getcontext had an error\n");
		exit(1);
	}

	void* stack = myMalloc(SIGSTKSZ);
	current.uc_link = NULL;
	current.uc_stack.ss_sp = stack;
	current.uc_stack.ss_size = SIGSTKSZ;
	current.uc_stack.ss_flags = 0;
	makecontext(&current, (void*)&runner, 2, function, arg);
	newThread->context = current;
	//Runqueue

	run_queue* newRunNode = myMalloc(sizeof(run_queue));
	newRunNode->threadControlBlock = newThread;
	newRunNode->next = NULL;

	if (runQueueHead == NULL)
	{
		runQueueHead = newRunNode;
	}
	else
	{
		run_queue* crnt = runQueueHead;

		while (crnt->next != NULL)
		{
			crnt = crnt->next;
		}

		crnt->next = newRunNode;
	}
	

    return newThread->t_id;
};

/* give CPU possession to other user-level threads voluntarily */
int mypthread_yield()
{
	// change thread state from Running to Ready
	// save context of this thread to its thread control block
	// wwitch from thread context to scheduler context

	// YOUR CODE HERE
	swapcontext(&(currentThread->context), &schedulerContext);
	return 0;
};

/* terminate a thread */
void mypthread_exit(void *value_ptr)
{
	// Deallocated any dynamic memory created when starting this thread

	// YOUR CODE HERE
	//put thread on finished stack
	finished_queue* finishedThread = myMalloc(sizeof(finished_queue));
	finishedThread->threadControlBlock->t_id = currentThread->t_id;
	finishedThread->value = value_ptr;
	if(finishedQueueHead == NULL)
	{
		finishedThread->next = NULL;
	}
	else{
		finishedThread->next=finishedQueueHead;
	}
	//set stack head to latest finished thread
	finishedQueueHead=finishedThread;
	done = 1;
	//go back to scheduler
	setcontext(&schedulerContext);
}


/* Wait for thread termination */
int mypthread_join(mypthread_t thread, void **value_ptr)
{
	// wait for a specific thread to terminate
	// de-allocate any dynamic memory created by the joining thread

	// YOUR CODE HERE
	while(1)
	{
		finished_queue* prev = NULL;
		finished_queue* curr = finishedQueueHead;
		//iterate through finished threads
		while(curr!=NULL)
		{
			//found the thread to join
			if(curr->threadControlBlock->t_id == thread)
			{
				if(curr->value != NULL)
				{
					value_ptr = &(curr->value);
				}
				//we are at the head
				if(prev == NULL)
				{
					finishedQueueHead = curr->next;
				}
				else{
					prev->next = curr->next;
				}
				free(curr);
				return 0;
			}
			prev = curr;
			curr = curr->next;
		}
		swapcontext(&(currentThread->context), &schedulerContext);
	}
	return 0;
};

/* initialize the mutex lock */
int mypthread_mutex_init(mypthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr)
{
	//initialize data structures for this mutex

	// YOUR CODE HERE
	mutex = myMalloc(sizeof(mypthread_mutex_t));
	mutex->locked = 0;
	mutex->t_id = -1;

	return 0;
};

/* aquire the mutex lock */
int mypthread_mutex_lock(mypthread_mutex_t *mutex)
{
	// use the built-in test-and-set atomic function to test the mutex
	// if the mutex is acquired successfully, enter the critical section
	// if acquiring mutex fails, push current thread into block list and //
	// context switch to the scheduler thread

	// YOUR CODE HERE
	if (mutex->locked == 1)
	{
		blocked_queue* newBlockedNode = myMalloc(sizeof(blocked_queue));
		newBlockedNode->threadControlBlock = currentThread;
		newBlockedNode->threadControlBlock->status = 2;
		newBlockedNode->next = NULL;

		if (blockedQueueHead == NULL)
		{
			blockedQueueHead = newBlockedNode;
		}
		else
		{
			blocked_queue* crnt = blockedQueueHead;

			while (crnt->next != NULL)
			{
				crnt = crnt->next;
			}

			crnt->next = newBlockedNode;
		}

		currentThread = NULL;
	}

	mutex->locked = 1;
	mutex->t_id = currentThread->t_id;
	
	return 0;
};

/* release the mutex lock */
int mypthread_mutex_unlock(mypthread_mutex_t *mutex)
{
	// Release mutex and make it available again.
	// Put threads in block list to run queue
	// so that they could compete for mutex later.

	// YOUR CODE HERE
	
	if (mutex->locked == 1 && mutex->t_id == currentThread->t_id && blockedQueueHead != NULL)
	{
		tcb* newRunThread = blockedQueueHead->threadControlBlock;

		run_queue* newRunNode = myMalloc(sizeof(run_queue));
		newRunNode->threadControlBlock = newRunThread;
		newRunNode->threadControlBlock->status = 1;
		newRunNode->next = NULL;

		if (runQueueHead == NULL)
		{
			runQueueHead = myMalloc(sizeof(run_queue));
			runQueueHead->threadControlBlock = newRunThread;
			runQueueHead->next = NULL;
		}
		else
		{
			run_queue* crnt = runQueueHead;
			
			while (crnt->next != NULL)
			{
				crnt = crnt->next;
			}

			crnt->next = newRunNode;
		}

		blocked_queue* oldBlockedNode = blockedQueueHead;
		blockedQueueHead = blockedQueueHead->next;

		free(oldBlockedNode);
	}

	return 0;
};


/* destroy the mutex */
int mypthread_mutex_destroy(mypthread_mutex_t *mutex)
{
	// Deallocate dynamic memory created in mypthread_mutex_init
	// free(mutex);

	return 0;
};

/* scheduler */
static void schedule()
{
	// Every time when timer interrup happens, your thread library
	// should be contexted switched from thread context to this
	// schedule function

	// Invoke different actual scheduling algorithms
	// according to policy (STCF or MLFQ)

	// if (sched == STCF)
	// 		sched_stcf();
	// else if (sched == MLFQ)
	// 		sched_mlfq();

	// YOUR CODE HERE

	// schedule policy
	#ifndef MLFQ
		sched_stcf();
	#else
		sched_mlfq();
	#endif

}

/* Preemptive SJF (STCF) scheduling algorithm */
static void sched_stcf()
{
	// Your own implementation of STCF
	// (feel free to modify arguments and return types)

	// YOUR CODE HERE
}

/* Preemptive MLFQ scheduling algorithm */
static void sched_mlfq()
{
	// Your own implementation of MLFQ
	// (feel free to modify arguments and return types)

	// YOUR CODE HERE
}

// Feel free to add any other functions you need

// YOUR CODE HERE
void runner(void*(*function)(void*), void* arg)
{
	function(arg);
}