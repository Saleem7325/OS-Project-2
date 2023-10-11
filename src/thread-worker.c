// File:	thread-worker.c

// List all group member's name:
// username of iLab:
// iLab Server:

#include "thread-worker.h"

//Global counter for total context switches and 
//average turn around and response time
long tot_cntx_switches=0;
double avg_turn_time=0;
double avg_resp_time=0;


// INITAILIZE ALL YOUR OTHER VARIABLES HERE

run_queue *rq = NULL;

/*_____________ run_queue functions ____________*/

void init_queue(run_queue *q){
	q->head, q->tail = NULL;
	q->size = 0;
}

void enqueue(run_queue *q, void  *data){
	if(q->tail == NULL){
		q->tail = malloc(sizeof(node));
		q->tail->data = data; 
		q->tail->next = NULL;
		q->head = q->tail;
	}else{
		node *tmp = malloc(sizeof(node));
		tmp->data = data;
		tmp->next = NULL;
		q->tail->next = tmp;
		q->tail = tmp;
	}
 
	q->size++;
}

void *dequeue(run_queue *q){
	if(q->head == NULL){
		return NULL;
	}else if(q->head == q->tail){
		void *data = q->head->data;
		free(q->head);

		q->head = NULL;
		q->tail = NULL; 
		q->size--;

		return data;    
	}

	void *data = q->head->data;
	node *tmp = q->head;
	q->head = q->head->next;
	free(tmp);
	q->size--;

	return data;    
}

/* TODO: Update function to free every dynamic mem-reference in TCB */
void free_queue(run_queue *q){
	node *tmp = q->head;
	while(tmp != NULL){
		node *prev = tmp;
		tmp = tmp->next;

		free(prev->data);
		free(prev); 
	}

	free(q);
}

run_queue *make_run_queue(){
	run_queue *q = malloc(sizeof(run_queue));	
	init_queue(q);

	return q;
}

/*_____________ worker_t functions ____________*/

/* create a new thread */
int worker_create(worker_t * thread, pthread_attr_t * attr, 
                      void *(*function)(void*), void * arg) {

       	// - create Thread Control Block (TCB)
       	// - create and initialize the context of this worker thread
	// - allocate space of stack for this thread to run
	//   after everything is set, push this thread into run queue and 
	// - make it ready for the execution.

	// Create a pointer to TCB
	tcb *control_block = malloc(sizeof(tcb));

	// Set thread ID
	control_block->thread_id = malloc(sizeof(worker_t));	
	*(control_block->thread_id) = *thread;

	// Set thread status 
	control_block->status = malloc(sizeof(int));	
	*(control_block->status) = READY;	

	// Set thread priority, using 1 as default for testing
	control_block->priority = malloc(sizeof(int));	
	*(control_block->priority) = 1;

	// Set up context for thread	
	ucontext_t *tctx = malloc(sizeof(ucontext_t));
	if(getcontext(tctx) < 0){
		// For test purposes only
		perror("worker_create: getcontext");
		exit(1);
	}

	// Allocate stack for context
	void *stack = malloc(STACK_SIZE);
	if(stack == NULL){
		// For test purposes only
		perror("worker_create: tcb stack allocation");
		exit(1);		
	}

	tctx->uc_link = NULL;
	tctx->uc_stack.ss_sp = stack;
	tctx->uc_stack.ss_size = STACK_SIZE;
	tctx->uc_stack.ss_flags = 0;	

	// Make the context start running at the function passed as arg	
	// TODO: getting a wanring from passing function, will need to 
	// resolve
	makecontext(tctx, function, 1, arg);
	control_block->context = tctx;
	
	// If run queue does not exist(in the case of first call),
	// create the run queue
	if(!rq){
		rq = make_run_queue();
	}

	// Push TCB onto run queue
	enqueue(rq, (void *)control_block);	
					
	return 0;
};

/* give CPU possession to other user-level worker threads voluntarily */
int worker_yield() {
		
	// - change worker thread's state from Running to Ready
	// - save context of this thread to its thread control block
	// - switch from thread context to scheduler context

	// YOUR CODE HERE

	//Checking if run queue exists. If no, no need to yield.
	if(!rq) {
		return -1;
	}

	//Checking if current thread exists (not the first call)
	if (rq->size == 0) {
		return -1;
	}

	//Save current thread's context
	tcb *current_thread = (tcb *)dequeue(rq);
	if (getcontext(current_thread->context) == -1) {
		//handle error
		perror("worker_yield: getcontext error");
		exit(1);
	}

	//Change the state of thread to READY.
	*(current_thread->status) = READY;

	//Enqueue the thread back to run queue, as it's still READY to run.
	enqueue(rq, (void *)current_thread);

	//switch from thread context to scheduler context here, comeback after scheduler done
	//setcontext(scheduler);
			
	return 0;
};

/* terminate a thread */
void worker_exit(void *value_ptr) {
	// - de-allocate any dynamic memory created when starting this thread
	
	// YOUR CODE HERE
};


/* Wait for thread termination */
int worker_join(worker_t thread, void **value_ptr) {
	
	// - wait for a specific thread to terminate
	// - de-allocate any dynamic memory created by the joining thread
  
	// YOUR CODE HERE
	return 0;
};

/* initialize the mutex lock */
int worker_mutex_init(worker_mutex_t *mutex, 
                          const pthread_mutexattr_t *mutexattr) {
	//- initialize data structures for this mutex

	// YOUR CODE HERE
	return 0;
};

/* aquire the mutex lock */
int worker_mutex_lock(worker_mutex_t *mutex) {

        // - use the built-in test-and-set atomic function to test the mutex
        // - if the mutex is acquired successfully, enter the critical section
        // - if acquiring mutex fails, push current thread into block list and
        // context switch to the scheduler thread

        // YOUR CODE HERE
        return 0;
};

/* release the mutex lock */
int worker_mutex_unlock(worker_mutex_t *mutex) {
	// - release mutex and make it available again. 
	// - put threads in block list to run queue 
	// so that they could compete for mutex later.

	// YOUR CODE HERE
	return 0;
};


/* destroy the mutex */
int worker_mutex_destroy(worker_mutex_t *mutex) {
	// - de-allocate dynamic memory created in worker_mutex_init

	return 0;
};

/* scheduler */
static void schedule() {
	// - every time a timer interrupt occurs, your worker thread library 
	// should be contexted switched from a thread context to this 
	// schedule() function

	// - invoke scheduling algorithms according to the policy (PSJF or MLFQ)

	// if (sched == PSJF)
	//		sched_psjf();
	// else if (sched == MLFQ)
	// 		sched_mlfq();

	// YOUR CODE HERE

// - schedule policy
#ifndef MLFQ
	// Choose PSJF
#else 
	// Choose MLFQ
#endif

}

/* Pre-emptive Shortest Job First (POLICY_PSJF) scheduling algorithm */
static void sched_psjf() {
	// - your own implementation of PSJF
	// (feel free to modify arguments and return types)

	// YOUR CODE HERE
}


/* Preemptive MLFQ scheduling algorithm */
static void sched_mlfq() {
	// - your own implementation of MLFQ
	// (feel free to modify arguments and return types)

	// YOUR CODE HERE
}

//DO NOT MODIFY THIS FUNCTION
/* Function to print global statistics. Do not modify this function.*/
void print_app_stats(void) {

       fprintf(stderr, "Total context switches %ld \n", tot_cntx_switches);
       fprintf(stderr, "Average turnaround time %lf \n", avg_turn_time);
       fprintf(stderr, "Average response time  %lf \n", avg_resp_time);
}


// Feel free to add any other functions you need

// YOUR CODE HERE

