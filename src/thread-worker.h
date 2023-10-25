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
#define QUANTUM 50000

/* Number of queues when scheduling with MLFQ */
#define MLFQ_SIZE 8

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
	int yeild_count;
	double start_time;
	double first_scheduled_time;
	double end_time;
	long context_switches;	
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

/*_________________________________ mutex_queue ___________________________________*/

typedef struct m_node{
	tcb *data;
	worker_mutex_t *mutex;
	struct m_node *next;
} m_node;

typedef struct mutex_queue{
	m_node *head;
	m_node *tail;
	int size;
} mutex_queue;

/* Function Declarations: */

void init_mutex_queue(mutex_queue *q);

void mutex_enqueue(mutex_queue *q, tcb  *data, worker_mutex_t *mutex);

void *mutex_dequeue(mutex_queue *q, worker_mutex_t *mutex);

void free_mutex_queue(mutex_queue *q);

/*_________________________________ join_queue ___________________________________*/

typedef struct j_node{
	tcb *data;
	worker_t child;
	struct j_node *next;
} j_node;

typedef struct join_queue{
	j_node *head;
	j_node *tail;
	int size;
} join_queue;

void init_join_queue(join_queue *q);

void join_enqueue(join_queue *q, tcb  *data, worker_t child);

void *join_dequeue(join_queue *q, worker_t child);

void free_join_queue(join_queue *q);

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

void init_list(list *lst);

void add(list *lst, worker_t data);

int get(list *lst, worker_t data);

void free_list(list *lst);

/* Function Declarations: */

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


//Function for get current time in microseconds.
double get_curr_time();
//Calculate all the global stats
void update_stats(double thread_turn_time, double thread_resp_time, long thread_cntx_switches);


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
