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
list *exit_list = NULL;

// Currently running thread's TCB
tcb *curr_tcb = NULL;
tcb *main_tcb = NULL;

// Scheduler context
ucontext_t sch_ctx;

// Timer for interupts
struct itimerval timer;

// sigaction for registering signal handler
struct sigaction sa;

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

/* TODO: Dequeue the node with minimum counter/elapsed value */
void *psjf_dequeue(run_queue *q){
		
}

/* TODO: Update function to free every dynamic mem-reference in TCB */
void free_queue(run_queue *q){
	node *tmp = q->head;
	while(tmp != NULL){
		node *prev = tmp;
		tmp = tmp->next;

		tcb *cb = (tcb *)prev->data;
		if(cb == main_tcb){
			free(prev);
			continue;
		}

		void *ctx_stk = cb->context.uc_stack.ss_sp;
		// Free stack, tcb, and node
		free(ctx_stk);
		free(prev->data);
		free(prev); 
	}

	free(q);
}

/*_______________ List functions _____________*/

void init_list(list *lst){
	lst->head = NULL;
	lst->size = 0;
}

void add(list *lst, worker_t data){
    l_node *tmp = (l_node *)malloc(sizeof(l_node));
    tmp->data = data;
    lst->size++;

    if(!lst->head){
        tmp->next = NULL;
        lst->head = tmp;
        return;
    }

    tmp->next = lst->head;
    lst->head = tmp;
}

int get(list *lst, worker_t data){
    if(!lst || !lst->head){
        return -1;
    }

    int ret = -1;
    if(lst->head->data == data){
        ret = lst->head->data;
       
        l_node *hd = lst->head;
        lst->head = lst->head->next;
        free(hd);
        lst->size--;

        return ret;
    }

    l_node *tmp = lst->head;
    l_node *next = tmp->next;

    while(next != NULL){
        if(next->data == data){
            ret = next->data;
            tmp->next = next->next;
            free(next);
             lst->size--;
            break;
        }

        tmp = next;
        next = next->next;
    }

    return ret;
}

void free_list(list *lst){
	l_node *tmp = lst->head;
	while(tmp != NULL){
		l_node *prev = tmp;
		tmp = tmp->next;
		free(prev); 
	}

	free(lst);
}

/*_____________ Set up functions ____________*/

static void schedule();

/* Creates context for scheduler */
void scheduler_context(){
	if(getcontext(&sch_ctx) < 0){
		// For test purposes only
		perror("worker_create: scheduler_context");
		exit(1);
	}

	void *stack = malloc(STACK_SIZE);
	if(stack == NULL){
		// For test purposes only
		perror("worker_create: scheduler stack allocation");
		exit(1);		
	}	
	
	sch_ctx.uc_link = NULL;
	sch_ctx.uc_stack.ss_sp = stack;
	sch_ctx.uc_stack.ss_size = STACK_SIZE;
	sch_ctx.uc_stack.ss_flags = 0;

	makecontext(&sch_ctx, &schedule, 0);
	// return sctx;
}

void set_timer(){
	timer.it_interval.tv_usec = 0;
	timer.it_interval.tv_sec = 0;
	timer.it_value.tv_usec = QUANTUM;
	timer.it_value.tv_sec = 0;
}

void init(){
	set_timer();

	// Create signal handler
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = &schedule;
	sigaction(SIGPROF, &sa, NULL);

	// create run queue
	rq = (run_queue *)malloc(sizeof(run_queue));	
	init_queue(rq);

	// create exit list
	exit_list =  (list *)malloc(sizeof(list));
	init_list(exit_list);

	// Get scheduler context
	// sch_ctx = *scheduler_context();
	scheduler_context();

	// Create a pointer to main TCB
	main_tcb = malloc(sizeof(tcb));
	main_tcb->thread_id = 0;
	main_tcb->status = READY;	
	main_tcb->priority = 1;
}

/*_____________ Helper functions ____________*/

void free_all(){
	free_queue(rq);
	free_list(exit_list);
	free(main_tcb);

	rq = NULL;
	main_tcb = NULL;
	exit_list = NULL;
	curr_tcb = NULL;
}

/*_____________ worker_t functions ____________*/

/* create a new thread */
int worker_create(worker_t * thread, pthread_attr_t * attr, void *(*function)(void*), void * arg) {
    // - create Thread Control Block (TCB)
    // - create and initialize the context of this worker thread
	// - allocate space of stack for this thread to run
	//   after everything is set, push this thread into run queue and 
	// - make it ready for the execution.

	// Create a pointer to TCB
	tcb *control_block = malloc(sizeof(tcb));

	// Set thread ID
	control_block->thread_id = *thread;	

	// Set thread status 
	control_block->status = READY;	

	// Set thread priority, using 1 as default for testing
	control_block->priority = 1;

	// Set up context for thread	
	// ucontext_t tctx;
	if(getcontext(&(control_block->context)) < 0){
		perror("worker_create: getcontext");
		exit(1);
	}

	// Allocate stack for context
	void *stack = malloc(STACK_SIZE);
	if(stack == NULL){
		perror("worker_create: tcb stack allocation");
		exit(1);		
	}

	// Set thread context attributes
	control_block->context.uc_link = NULL; 
	control_block->context.uc_stack.ss_sp = stack;
	control_block->context.uc_stack.ss_size = STACK_SIZE;
	control_block->context.uc_stack.ss_flags = 0;		

	// Make the context start running at the function passed as arg	
	makecontext(&(control_block->context), (void *)function, 1, arg);
	// control_block->context = tctx;

	// If run queue does not exist(in the case of first call),
	// create the run queue
	if(!rq){
		init();

		if(getcontext(&(main_tcb->context)) < 0){
			perror("worker_create: main getcontext");
			exit(1);
		}

		// Check is current context was a result of setcontext()
		if(curr_tcb != NULL && curr_tcb->status == SCHEDULED){
			printf("Made it");
			return 0;
		}else{
			// Push TCB onto run queue
			enqueue(rq, (void *)control_block);
			enqueue(rq, (void *)main_tcb);
		}
	}else{
		enqueue(rq, (void *)control_block);
	}

	// Continue executing thread
	if(curr_tcb != NULL && curr_tcb->status == SCHEDULED){
		return 0;
	}	
	
	// Switch to scheduler context					
	setcontext(&sch_ctx);
	// schedule();
	
	return 0;
};

/* give CPU possession to other user-level worker threads voluntarily */
int worker_yield() {
	// - change worker thread's state from Running to Ready
	// - save context of this thread to its thread control block
	// - switch from thread context to scheduler context

	// Checking if run queue exists. If no, no need to yield.
	if(!rq) {
		return -1;
	}

	// If no other threads to run, continue running thread
	if (rq->size == 0) {
		return -1;
	}

	// Sanity check
	if(!curr_tcb){
		printf("worker_yield: current tcb is null");
		return -1;
	}

	// Update status of yeilding thread
	curr_tcb->status = READY;
	if(getcontext(&(curr_tcb->context)) < 0){
		perror("worker_yield: getcontext error");
		exit(1);
	}

	// If curr_tcb is set again using setcontext
	if(curr_tcb->status == SCHEDULED){
		return 0;
	}

	// Add yielding thread to run_queue
	enqueue(rq, (void *)curr_tcb);
	curr_tcb = NULL;

	//switch from thread context to scheduler context
	setcontext(&sch_ctx);
	// schedule();
			
	return 0;
};

/* terminate a thread */
void worker_exit(void *value_ptr) {
	// - add thread id to exit list
	add(exit_list, curr_tcb->thread_id);

	// - de-allocate any dynamic memory created when starting this thread
	free(curr_tcb->stack);
	free(curr_tcb);
	curr_tcb = NULL;
	
	//TODO: save value of arg, need to update list
	// schedule();
	setcontext(&sch_ctx);
};


/* Wait for thread termination */
int worker_join(worker_t thread, void **value_ptr) {
	
	// - wait for a specific thread to terminate
	// - de-allocate any dynamic memory created by the joining thread
  
	// YOUR CODE HERE

	// While the thread we are waiting on is not in the exit list
	// yeild to give other threads in run queue CPU resource
	while(get(exit_list, thread) == -1){
		worker_yield();
	}

	// Need to get return value

	// Assuming yield de-allocates all tcb memory nothing left to
	// if(curr_tcb == main_tcb && rq->size == 0){
	// 	free_all();
	// }
	
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

/* scheduler currently implements cfs and does not call MLFQ/PSJF */
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
	// if (getcontext(&sch_ctx) < 0){
	// 	perror("schedule: getcontext");
	// 	exit(1);
	// }
	while(1){
		if(curr_tcb != NULL && curr_tcb->status == SCHEDULED){
			return;
		}

		if(!rq){
			return;
		}

		if(curr_tcb == NULL && rq->size == 0){
			free_list(exit_list);
			free_queue(rq);
			return;
		}

		// If no thread has been interrupted by timer dequeue
		// a thread and start executing its context
		if(curr_tcb == NULL && rq->size > 0){
			curr_tcb = (tcb *)dequeue(rq);	
			curr_tcb->status = SCHEDULED;		
		}else if (rq->size > 0){
			curr_tcb->status = READY;
			enqueue(rq, curr_tcb);
			
			curr_tcb = (tcb *)dequeue(rq);
			curr_tcb->status = SCHEDULED;
		}

		set_timer();	
		setitimer(ITIMER_PROF, &timer, NULL);
		setcontext(&(curr_tcb->context));
	}	
	// swapcontext(&sch_ctx, &(curr_tcb->context));
	// setcontext(&sch_ctx);

		

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

