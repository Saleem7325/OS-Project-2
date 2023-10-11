#include <stdlib.h>
#include <stdio.h>

typedef struct node{
	void *data;
	struct node *next;
}node;

/*
 * Generic type linked-list based queue which maintains a pointer 
 * to head and tail for constant time dequeue and enqueue operations 
*/
typedef struct run_queue{
	node *head;
	node *tail;
	int size;
}run_queue;

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

/* Just for testing */
int main(int argc, char **argv){
	run_queue *rq = malloc(sizeof(run_queue));
	init_queue(rq);	

	for(int i = 0; i < 30; i++){
		int *num = malloc(sizeof(int));
		*num = i;
		enqueue(rq, (void *)num);
		printf("SIZE: %d\n", rq->size);
	}

/*	for(int i = 0; i < 33; i++){
		int *num = (int *)dequeue(rq); 
		if(num == NULL){
			printf("NULL\n");
			continue;
		}
		printf("%d\n", *num);
		free(num);
		printf("SIZE: %d\n", rq->size);
	}
*/
	free_queue(rq);	
}
