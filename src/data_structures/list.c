#include <stdlib.h>
#include <stdio.h>

typedef uint worker_t;

typedef struct l_node{
	worker_t data;
	struct l_node *next;
} l_node;

/*
 * A linked-list based list implementation which constant time add operation
*/
typedef struct list{
	l_node *head;
	int size;
} list;

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

/* Just for testing */
int main(int argc, char **argv){
	list *lst = malloc(sizeof(list));
	init_list(lst);	

	for(int i = 0; i < 30; i++){
		add(lst, (worker_t)i);
		printf("SIZE: %d\n", lst->size);
	}

	for(int i = 0; i < 33; i++){
		printf("%d\n", get(lst, i));
		printf("SIZE: %d\n", lst->size);
	}

	free_list(lst);	
}