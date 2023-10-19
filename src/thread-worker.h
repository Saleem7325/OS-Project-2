// File:	worker_t.h

// List all group member's name:
// username of iLab:
// iLab Server:

#ifndef WORKER_T_H
#define WORKER_T_H

#define _GNU_SOURCE

/* To use Linux pthread Library in Benchmark, you have to comment the USE_WORKERS macro */
#define USE_WORKERS 1

/* worker_t statuses */
#define READY 0
#define SCHEDULED 1
#define BLOCKED 2

/* Minimum number of microsends a thread can run before context switch */
#define QUANTUM 10000

/* Stack Size for TCB */
#define STACK_SIZE SIGSTKSZ

/* include lib header files that you need here: */
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>

/* Were not included in unedited file */
#include <ucontext.h>
#include <signal.h>
#include <sys/time.h>
#include <string.h>


typedef uint worker_t;

typedef struct TCB {
	/* add important states in a thread control block */
	// thread Id
	// thread status
	// thread context
	// thread stack
	// thread priority
	// And more ...

	worker_t thread_id;
	int status;
	ucontext_t context;
	void *stack;
	int priority;
	int elapsed;	
} tcb; 

/* mutex struct definition */
typedef struct worker_mutex_t {
	/* add something here */
	// YOUR CODE HERE

	int locked;
	tcb *holder;

} worker_mutex_t;

/* define your data structures here: */
// Feel free to add your own auxiliary data structures (linked list or queue etc...)

/*_________________________________ run_queue ___________________________________*/

/* Node in linked list based queue */
typedef struct node{
	void *data;
	struct node *next;
} node;

/*
 * Generic type linked-list based queue which maintains a pointer 
 * to head and tail for constant time dequeue and enqueue operations 
*/
typedef struct run_queue{
	node *head;
	node *tail;
	int size;
} run_queue;

/*_________________________________ blocked_queue ___________________________________*/

typedef struct b_node{
	tcb *data;
	worker_mutex_t *mutex;
	struct b_node *next;
} b_node;

typedef struct blocked_queue{
	b_node *head;
	b_node *tail;
	int size;
} blocked_queue;

/*_________________________________ list ___________________________________*/

/* Node in list */
typedef struct l_node{
	worker_t data;
	struct l_node *next;
} l_node;

/*
 * A linked-list based list implementation with constant time add operation
*/
typedef struct list{
	l_node *head;
	int size;
} list;

/* Function Declarations: */

/* Given a pointer to a run_queue, function sets head/tail to NULL and size to 0 */
void init_queue(run_queue *q);

/* Add data to the back of the queue */
void enqueue(run_queue *q,  void *data);

/* Remove data from the front of the queue and return a pointer to the data
 * if run_queue is empty returns NULL 
 */
void *dequeue(run_queue *q);

/* Frees all data and nodes as well as freeing the queue */
void free_queue(run_queue *q);

/* Creates a run queue struct and initializes its values 
 * returns a pointer to the run_queue that was created
 */
run_queue *make_run_queue();

/* create a new thread */
int worker_create(worker_t * thread, pthread_attr_t * attr, void
    *(*function)(void*), void * arg);

/* give CPU pocession to other user level worker threads voluntarily */
int worker_yield();

/* terminate a thread */
void worker_exit(void *value_ptr);

/* wait for thread termination */
int worker_join(worker_t thread, void **value_ptr);

/* initial the mutex lock */
int worker_mutex_init(worker_mutex_t *mutex, const pthread_mutexattr_t
    *mutexattr);

/* aquire the mutex lock */
int worker_mutex_lock(worker_mutex_t *mutex);

/* release the mutex lock */
int worker_mutex_unlock(worker_mutex_t *mutex);

/* destroy the mutex */
int worker_mutex_destroy(worker_mutex_t *mutex);


/* Function to print global statistics. Do not modify this function.*/
void print_app_stats(void);

#ifdef USE_WORKERS
#define pthread_t worker_t
#define pthread_mutex_t worker_mutex_t
#define pthread_create worker_create
#define pthread_exit worker_exit
#define pthread_join worker_join
#define pthread_mutex_init worker_mutex_init
#define pthread_mutex_lock worker_mutex_lock
#define pthread_mutex_unlock worker_mutex_unlock
#define pthread_mutex_destroy worker_mutex_destroy
#endif

#endif
