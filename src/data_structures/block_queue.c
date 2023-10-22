#include <stdio.h>
#include <stdlib.h>

typedef struct b_node{
    int data;
	struct b_node *next;
} b_node;

typedef struct blocked_queue{
	b_node *head;
	b_node *tail;
	int size;
} blocked_queue;

blocked_queue *bq = NULL;

void init_b_queue(blocked_queue *q){
	q->head, q->tail = NULL;
	q->size = 0;
}

void b_enqueue(blocked_queue *q, int data){
	if(q->tail == NULL){
		q->tail = malloc(sizeof(b_node));
		q->tail->data = data;
		q->tail->next = NULL;
		q->head = q->tail;
	}else{
		b_node *tmp = malloc(sizeof(b_node));
		tmp->data = data; 
		tmp->next = NULL;
		q->tail->next = tmp;
		q->tail = tmp;
	}
 
	q->size++;
}

int b_dequeue(blocked_queue *q, int data){
	if(q->head == NULL){
		return -1;
	}
	
	if(q->head->data == data){
		int data = q->head->data;

		if(q->head == q->tail){
			free(q->head);
			q->head = NULL;
			q->tail = NULL; 

		}else{
			b_node *tmp = q->head;
			q->head = q->head->next;
			free(tmp);
		}

		q->size--;

		return data;    
	}
	
	b_node *tmp = q->head;
	b_node *next = tmp->next;
	while(next != NULL){
		if(next->data == data){
			if(next == q->tail){
				int data = next->data;
				q->tail = tmp;
				q->tail->next = NULL;
			}else{
				int data = next->data;
				tmp->next = next->next;
			}
	
			q->size--;
			free(next);
			return data;
		}

		tmp = next;
		next = next->next;
	}

	return -1;   
}

void free_queue(blocked_queue *q){
	b_node *tmp = q->head;
	while(tmp != NULL){
		b_node *prev = tmp;
		tmp = tmp->next;
		free(prev); 
	}

	free(q);
}

int main(int argc, char **argv){
	blocked_queue *bq = malloc(sizeof(blocked_queue));
	init_b_queue(bq);	

	for(int i = 0; i < 10000; i++){
		b_enqueue(bq, i);
		printf("SIZE: %d\n", bq->size);
	}

	for(int i = 10000; i >= -1; i--){
		printf("%d\n", b_dequeue(bq, i));
		printf("SIZE: %d\n", bq->size);
	}

	free_queue(bq);
}