// File:	thread-worker.c

// List all group member's name:
// username of iLab:
// iLab Server:

#include "thread-worker.h"

// Scheduling Policy
#ifndef MLFQ
#define POLICY 1 /* PSJF */
#else 
#define POLICY 0 /* MLFQ */
#endif

//Global counter for total context switches and 
//average turn around and response time
long tot_cntx_switches=0;
double avg_turn_time=0;
double avg_resp_time=0;

int total_threads_completed = 0;

// INITAILIZE ALL YOUR OTHER VARIABLES HERE

// PSJF run queue
run_queue *rq = NULL;

// MLFQ run_queue
run_queue **mlfq = NULL;

// Blocked threads
mutex_queue *mq = NULL;
join_queue *jq = NULL;

// List of completed threads
list *exit_list = NULL;

// Currently running thread's TCB
tcb *curr_tcb = NULL;

// Main thread's TCB
tcb *main_tcb = NULL;

// Scheduler context
ucontext_t sch_ctx;

// Timer for interupts
struct itimerval timer;

// sigaction for registering signal handler
struct sigaction sa;

// Thread_IDs
int id = 1;

/*_____________ run_queue functions ____________*/

void init_queue(run_queue *q){
	q->head = NULL;
	q->tail = NULL;
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

void *psjf_dequeue(run_queue *q){
		if(!rq){
			return NULL;
		}

		node *tmp = q->head;
		tcb *tmp_tcb = (tcb *)tmp->data;
		node *next = tmp->next;

		tcb *min_tcb = (tcb *)tmp->data;
		node *min_node = tmp;
		node *min_prev = NULL;
		node *min_next = next;

		if(tmp_tcb->elapsed == 0 || tmp == q->tail){
			if(tmp == q->tail){
				q->head = NULL;
				q->tail = NULL;
			}else{
				q->head = next;
			}

			q->size--;
			free(tmp);
			return tmp_tcb;
		}

		while(next != NULL){
			tcb * next_tcb = (tcb *)next->data;
			if(next_tcb->elapsed == 0){
				min_tcb = next_tcb;

				if(next == q->tail){
					tmp->next = NULL;
					q->tail = tmp;
				}else{
					tmp->next = next->next;
				}

				free(next);
				return min_tcb;
			}else if(next_tcb->elapsed < min_tcb->elapsed){
				min_prev = tmp;
				min_next = next->next;
				min_node = next;
				min_tcb = next_tcb;
			}

			tmp = next;
			next = next->next;
		}

		if(min_node == q->head){
			q->head = min_next;
		}else if(min_node == q->tail){
			q->tail = min_prev;
			q->tail->next = NULL;
		}else{
			min_prev->next = min_next;
		}

		q->size--;
		free(min_node);
		return min_tcb;


}

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

void print_queue(run_queue *rq){
	if(!rq){
		return;
	}

	if(rq->head == rq->tail){
		printf("Size: %d\n", rq->size);

		if(!rq->head){
			return;
		}

		tcb * tmp = (tcb *)rq->head->data;
		int tmp_id = (int)tmp->thread_id;
		printf("Thread_id: %d\n", tmp_id);
		return;
	}

	node * tmp = rq->head;
	while(!tmp){
		tcb *data = (tcb *)rq->head->data;
		int tmp_id = (int)data->thread_id;
		printf("Thread_id: %d\n", tmp_id);
		tmp = tmp->next;
	}

}

/*_______________________ mutex_queue functions ___________________________*/

void init_mutex_queue(mutex_queue *q){
	q->head = NULL;
	q->tail = NULL;
	q->size = 0;
}

void mutex_enqueue(mutex_queue *q, tcb  *data, worker_mutex_t *mutex){
	if(q->tail == NULL){
		q->tail = malloc(sizeof(m_node));
		q->tail->data = data;
		q->tail->mutex = mutex; 
		q->tail->next = NULL;
		q->head = q->tail;
	}else{
		m_node *tmp = malloc(sizeof(m_node));
		tmp->data = data;
		tmp->mutex = mutex; 
		tmp->next = NULL;
		q->tail->next = tmp;
		q->tail = tmp;
	}
 
	q->size++;
}

void *mutex_dequeue(mutex_queue *q, worker_mutex_t *mutex){
	if(q->head == NULL){
		return NULL;
	}
	
	if(q->head->mutex == mutex){
		void *data = q->head->data;

		if(q->head == q->tail){
			free(q->head);
			q->head = NULL;
			q->tail = NULL; 

		}else{
			m_node *tmp = q->head;
			q->head = q->head->next;
			free(tmp);
		}

		q->size--;

		return data;    
	}

	m_node *tmp = q->head;
	m_node *next = tmp->next;
	while(next != NULL){
		if(next->mutex == mutex){
			void *data = next->data;

			if(next == q->tail){
				q->tail = tmp;
			}else{
				tmp->next = next->next;
			}

			free(next);
			return data;
		}

		tmp = next;
		next = next->next;
	}

	return NULL;   
}

void free_mutex_queue(mutex_queue *q){
	m_node *tmp = q->head;
	while(tmp != NULL){
		m_node *prev = tmp;
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

/*_______________________ join_queue functions ___________________________*/

void init_join_queue(join_queue *q){
	q->head = NULL;
	q->tail = NULL;
	q->size = 0;
}

void join_enqueue(join_queue *q, tcb  *data, worker_t child){
	if(q->tail == NULL){
		q->tail = malloc(sizeof(j_node));
		q->tail->data = data;
		q->tail->child = child; 
		q->tail->next = NULL;
		q->head = q->tail;
	}else{
		j_node *tmp = malloc(sizeof(j_node));
		tmp->data = data;
		tmp->child = child; 
		tmp->next = NULL;
		q->tail->next = tmp;
		q->tail = tmp;
	}
 
	q->size++;
}

void *join_dequeue(join_queue *q, worker_t child){
	if(!q || q->head == NULL){
		return NULL;
	}
	
	if(q->head->child == child){
		void *data = q->head->data;

		if(q->head == q->tail){
			free(q->head);
			q->head = NULL;
			q->tail = NULL; 

		}else{
			j_node *tmp = q->head;
			q->head = q->head->next;
			free(tmp);
		}

		q->size--;
		return data;    
	}

	j_node *tmp = q->head;
	j_node *next = tmp->next;
	while(next != NULL){
		if(next->child == child){
			void *data = next->data;

			if(next == q->tail){
				q->tail = tmp;
			}else{
				tmp->next = next->next;
			}

			free(next);
			return data;
		}

		tmp = next;
		next = next->next;
	}

	return NULL;  
}

void free_join_queue(join_queue *q){
	j_node *tmp = q->head;
	while(tmp != NULL){
		j_node *prev = tmp;
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

/*______________________________ list functions ________________________________*/

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

/*_____________ Multi level run queue functions _____________*/

void init_mlfq(){
	for(int i = 0; i < MLFQ_SIZE; i++){
		mlfq[i] = (run_queue *)malloc(sizeof(run_queue));
		init_queue(mlfq[i]);
	}
}

int mlfq_size(){
	if(!mlfq){
		return 0;
	}

	int size = 0;
	for(int i = 0; i < MLFQ_SIZE; i++){
		size += mlfq[i]->size; 
	}

	return size;
}

void reset_blocked_priority(){
	if(mq != NULL && mq->size > 0){
		m_node *tmp = mq->head;

		while(tmp != NULL){
			tmp->data->elapsed = 0;
			tmp = tmp->next;
		}
	}

	if(jq != NULL && jq->size > 0){
		j_node *tmp = jq->head;
		
		while(tmp != NULL){
			tmp->data->elapsed = 0;
			tmp = tmp->next;
		}
	}
}

void reset_priority(){
	run_queue *top_q = mlfq[0];
	for(int i = 1; i < MLFQ_SIZE; i++){
		run_queue *q = mlfq[i];

		if(!q || q->size == 0){
			continue;
		}

		tcb *cb = (tcb *)dequeue(q);
		while(cb != NULL){
			cb->priority = 0;
			enqueue(top_q, (void *)cb);
			cb = (tcb *)dequeue(q);
		}
	}

	reset_blocked_priority();
}

void free_mlfq(){
	for(int i = 0; i < MLFQ_SIZE; i++){
		free_queue(mlfq[i]); 
	}

	free(mlfq);
}

/*_____________ Set up functions ____________*/

void sig_handle(int sig_num);

static void schedule();

static void sched_psjf();

static void sched_mlfq();

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

void disable_timer(){
	timer.it_interval.tv_usec = 0;
	timer.it_interval.tv_sec = 0;
	timer.it_value.tv_usec = 0;
	timer.it_value.tv_sec = 0;
	setitimer(ITIMER_PROF, &timer, NULL);
}

void init(){
	set_timer();

	// Create signal handler
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = &sig_handle;
	sigaction(SIGPROF, &sa, NULL);

	// create run queue
	if(POLICY){
		rq = (run_queue *)malloc(sizeof(run_queue));	
		init_queue(rq);
	}else{
		mlfq = (run_queue **)malloc(sizeof(run_queue *) * MLFQ_SIZE);
		init_mlfq(mlfq);
	}

	// create mutex queue
	mq = (mutex_queue *)malloc(sizeof(mutex_queue));	
	init_mutex_queue(mq);

	// create join queue
	jq = (join_queue *)malloc(sizeof(join_queue));	
	init_join_queue(jq);

	// create exit list
	exit_list =  (list *)malloc(sizeof(list));
	init_list(exit_list);

	// Get scheduler context
	scheduler_context();

	// Create a pointer to main TCB
	main_tcb = malloc(sizeof(tcb));
	main_tcb->thread_id = 0;
	main_tcb->status = READY;	
	main_tcb->priority = 0;
	main_tcb->elapsed = 0;
}

/*_____________ Helper functions ____________*/

void free_all(){
	disable_timer(); 
	
	if(POLICY){
		free_queue(rq);
	}else{
		free_mlfq();
	}
	
	free_mutex_queue(mq);
	free_join_queue(jq);
	free_list(exit_list);
	free(main_tcb);
	free(sch_ctx.uc_stack.ss_sp);

	rq = NULL;
	mlfq = NULL;
	mq = NULL;
	jq = NULL;
	main_tcb = NULL;
	exit_list = NULL;
	curr_tcb = NULL;
	id = 1;
}

int total_threads(){
	if(POLICY){
		return rq->size + mq->size + jq->size + exit_list->size; 
	}else{
		return mlfq_size() + mq->size + jq->size + exit_list->size;
	}
}


double get_curr_time() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000.0 + tv.tv_usec / 1000.0;
}

//Update all the stats
void update_stats(double thread_turn_time, double thread_resp_time, long thread_cntx_switches) {
	//Update context switches
	tot_cntx_switches += thread_cntx_switches;

	//Update each metrics averages
	avg_turn_time = (avg_turn_time * total_threads_completed + thread_turn_time) / (total_threads_completed + 1);
	avg_resp_time = (avg_resp_time * total_threads_completed + thread_resp_time) / (total_threads_completed + 1);

	//Increment the count of completed threads
	total_threads_completed++;

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
	// control_block->thread_id = *thread;
	*thread = id++;
	control_block->thread_id = *thread;	


	// Set thread status 
	control_block->status = READY;	

	// Set thread priority
	control_block->priority = 0;
	control_block->elapsed = 0;

	//Stats
	control_block->start_time = get_curr_time();  // Capture the current time in microseconds
    control_block->first_scheduled_time = 0;  // Not scheduled yet
    control_block->end_time = 0;  // Not finished yet
    control_block->context_switches = 0;  // No context switches at creation

	// Set up context for thread	
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

	// If run queue does not exist(in the case of first call),
	// create the run queue
	if(!rq && !mlfq){
		init();

		if(getcontext(&(main_tcb->context)) < 0){
			perror("worker_create: main getcontext");
			exit(1);
		}

		// Check is current context was a result of setcontext()
		if(curr_tcb != NULL && curr_tcb->status == SCHEDULED){
			return 0;
		}
		
		if(POLICY){
			// Push TCB onto run queue
			enqueue(rq, (void *)control_block);
			enqueue(rq, (void *)main_tcb);
		}else{
			enqueue(mlfq[0], (void *)control_block);
			enqueue(mlfq[0], (void *)main_tcb);
		}

	}else if(POLICY){
		enqueue(rq, (void *)control_block);
	}else{
		enqueue(mlfq[0], (void *)control_block);
	}

	// Continue executing thread
	if(curr_tcb != NULL && curr_tcb->status == SCHEDULED){
		return 0;
	}	
	
	// Switch to scheduler context					
	setcontext(&sch_ctx);
	
	return 0;
};

/* give CPU possession to other user-level worker threads voluntarily */
int worker_yield() {
	// - change worker thread's state from Running to Ready
	// - save context of this thread to its thread control block
	// - switch from thread context to scheduler context

	// Checking if run queue exists. If no, no need to yield.
	if(!rq && !mlfq) {
		return -1;
	}

	// If no other threads to run, continue running thread
	if (rq->size == 0 && mlfq_size == 0) {
		return -1;
	}

	// Sanity check
	if(!curr_tcb){
		printf("worker_yield: current tcb is null");
		return -1;
	}

	disable_timer();

	//Update context switch count for yielding thread
	curr_tcb->context_switches++;
	if(POLICY && ++(curr_tcb->yeild_count) % 2 == 0){
		curr_tcb->elapsed++;
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
	if(POLICY){
		enqueue(rq, (void *)curr_tcb);
	}else{
		enqueue(mlfq[curr_tcb->priority], (void *)curr_tcb);	
	}

	curr_tcb = NULL;

	//switch from thread context to scheduler context
	setcontext(&sch_ctx);
			
	return 0;
};

/* terminate a thread */
void worker_exit(void *value_ptr) {
	// - add thread id to exit list
	disable_timer();
	//printf("Finished thread %d,	\telapsed: %d\n", (int)curr_tcb->thread_id, curr_tcb->elapsed);
	//get thread end time and update time
	curr_tcb->end_time = get_curr_time();

	//Calculate individual thread's turn time and resp time
	double ind_turn_time = curr_tcb->end_time - curr_tcb->start_time;
	double ind_resp_time = curr_tcb->first_scheduled_time - curr_tcb->start_time;

	update_stats(ind_turn_time, ind_resp_time, curr_tcb->context_switches);

	add(exit_list, curr_tcb->thread_id);

	tcb *parent = (tcb *)join_dequeue(jq, curr_tcb->thread_id);
	if(parent != NULL){
		parent->status = READY;

		if(POLICY){
			enqueue(rq, (void *)parent);
		}else{
			enqueue(mlfq[curr_tcb->priority], (void *)parent);
		}
		
	}

	// - de-allocate any dynamic memory created when starting this thread
	free(curr_tcb->stack);
	free(curr_tcb);
	curr_tcb = NULL;

	setcontext(&sch_ctx);
};


/* Wait for thread termination */
int worker_join(worker_t thread, void **value_ptr) {
	
	// - wait for a specific thread to terminate
	// - de-allocate any dynamic memory created by the joining thread
  
	// YOUR CODE HERE
	if(getcontext(&(curr_tcb->context)) < 0){
		perror("worker_join: getcontext");
		exit(1);
	}

	while(get(exit_list, thread) == -1){
		disable_timer();

		curr_tcb->status = BLOCKED;
		join_enqueue(jq, curr_tcb, thread);
		curr_tcb = NULL;

		setcontext(&sch_ctx);
	}

	if(curr_tcb == main_tcb && total_threads() == 0){
		free_all();
	}
	
	return 0;
};

/* initialize the mutex lock */
int worker_mutex_init(worker_mutex_t *mutex, const pthread_mutexattr_t *mutexattr) {
	//- initialize data structures for this mutex
	mutex->locked = 0;
	mutex->holder = NULL;

	// YOUR CODE HERE
	return 0;
};

/* aquire the mutex lock */
int worker_mutex_lock(worker_mutex_t *mutex) {
	if(mutex->holder == curr_tcb){
		return 0;
	}

	// - use the built-in test-and-set atomic function to test the mutex
	// - if the mutex is acquired successfully, enter the critical section
	// - if acquiring mutex fails, push current thread into block list and
	// context switch to the scheduler thread

	// YOUR CODE HERE
	if(getcontext(&(curr_tcb->context)) < 0){
		perror("worker_mutex_lock: getcontext");
		exit(1);
	}

	while(__sync_lock_test_and_set(&mutex->locked, 1) != 0){
		disable_timer();

		curr_tcb->status = BLOCKED;
		mutex_enqueue(mq, curr_tcb, mutex);
		curr_tcb = NULL;

		setcontext(&sch_ctx);
	}
	
	mutex->holder = curr_tcb;
	return 0;
};

/* release the mutex lock */
int worker_mutex_unlock(worker_mutex_t *mutex) {
	// - release mutex and make it available again. 
	// - put threads in block list to run queue 
	// so that they could compete for mutex later.

	// YOUR CODE HERE
	mutex->locked = 0;
	mutex->holder = NULL;

	tcb *thread = mutex_dequeue(mq, mutex);
	if(thread != NULL){
		thread->status = READY;

		if(POLICY){
			enqueue(rq, (void *)thread);
		}else{
			enqueue(mlfq[thread->priority], (void *)thread);
		}
		
	}

	return 0;
};


/* destroy the mutex */
int worker_mutex_destroy(worker_mutex_t *mutex) {
	// - de-allocate dynamic memory created in worker_mutex_init
	mutex->locked = 0;
	mutex->holder = NULL;
	return 0;
};

void sig_handle(int sig_num){
	if(sig_num != 27){
		printf("Signal num not 27\n");
		return;
	}

	if(!curr_tcb){
		setcontext(&sch_ctx);
	}

	if(POLICY){
		curr_tcb->elapsed++;
	}else if(curr_tcb->priority < (MLFQ_SIZE - 1)){
		curr_tcb->priority++;
	}

	curr_tcb->status = READY;
	if(getcontext(&(curr_tcb->context)) < 0){
		perror("worker_yield: getcontext error");
		exit(1);
	}

	// If curr_tcb is set again using setcontext
	if(curr_tcb->status == SCHEDULED){
		return;
	}

	// printf("Timer Interrupt: Thread %d\n", (int)curr_tcb->thread_id);
	if(POLICY){
		enqueue(rq, curr_tcb);
	}else{
		enqueue(mlfq[curr_tcb->priority], curr_tcb);
	}

	curr_tcb = NULL;
	setcontext(&sch_ctx);
}

/* scheduler currently implements cfs and does not call MLFQ/PSJF */
static void schedule() {
	// - every time a timer interrupt occurs, your worker thread library 
	// should be contexted switched from a thread context to this 
	// schedule() function

	// - invoke scheduling algorithms according to the policy (PSJF or MLFQ)
	// YOUR CODE HERE
	if(POLICY){
		sched_psjf();
	}else{
		sched_mlfq();
	}	
		
// - schedule policy
// #ifndef MLFQ
// 	// Choose PSJF
// #else 
// 	// Choose MLFQ
// #endif

}

/* Pre-emptive Shortest Job First (POLICY_PSJF) scheduling algorithm */
static void sched_psjf() {
	// - your own implementation of PSJF
	// (feel free to modify arguments and return types)
	// YOUR CODE HERE
	while(1){
		if(!rq){
			free_all();
			return;
		}

		if(curr_tcb == NULL && rq->size == 0){
			free_all();
			return;
		}

		// If no thread has been interrupted by timer dequeue
		// a thread and start executing its context
		if(curr_tcb == NULL && rq->size > 0){
			curr_tcb = (tcb *)psjf_dequeue(rq);	
			curr_tcb->status = SCHEDULED;		
		}else if (rq->size > 0){
			curr_tcb->status = READY;
			enqueue(rq, curr_tcb);
			
			curr_tcb = (tcb *)psjf_dequeue(rq);
			curr_tcb->status = SCHEDULED;
		}

		// Update the statistics for the currently scheduled thread
        if (curr_tcb->first_scheduled_time == 0) {
            curr_tcb->first_scheduled_time = get_curr_time();
        }
        curr_tcb->context_switches++;

		// printf("Running Thread %d\n", (int)curr_tcb->thread_id);
		set_timer();	
		setitimer(ITIMER_PROF, &timer, NULL);
		setcontext(&(curr_tcb->context));
	}
}


/* Preemptive MLFQ scheduling algorithm */
static void sched_mlfq() {
	// - your own implementation of MLFQ
	// (feel free to modify arguments and return types)
	// YOUR CODE HERE
	int prev_dequeue_priority = 0;
	int priority_count = 0;

	while(1){
		if(!mlfq){
			// free_all();
			return;
		}

		if(curr_tcb == NULL && mlfq_size() == 0){
			free_all();
			return;
		}

		if(curr_tcb != NULL){
			curr_tcb->status = READY;
			enqueue(mlfq[curr_tcb->priority], curr_tcb);
		}

		for(int i = 0; i < MLFQ_SIZE; i++){
			curr_tcb = (tcb *)dequeue(mlfq[i]);

			if(!curr_tcb){
				continue;
			}

			curr_tcb->status = SCHEDULED;

			if(i == prev_dequeue_priority){
				priority_count++;
				break;
			}else{
				prev_dequeue_priority = i;
				priority_count = 1;
				break;
			}
		}

		if(prev_dequeue_priority == (MLFQ_SIZE - 1) && priority_count == total_threads() * 2){
			curr_tcb->priority = 0;
			reset_priority();
			prev_dequeue_priority = 0;
			priority_count = 0;
			// printf("Resetting Priority\n");
		}

		// Update the statistics for the currently scheduled thread
        if (curr_tcb->first_scheduled_time == 0) {
            curr_tcb->first_scheduled_time = get_curr_time();
        }
        curr_tcb->context_switches++;

		// printf("Running Thread %d\n", (int)curr_tcb->thread_id);
		set_timer();	
		setitimer(ITIMER_PROF, &timer, NULL);
		setcontext(&(curr_tcb->context));
	}
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

