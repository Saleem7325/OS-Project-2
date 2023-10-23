#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include "../thread-worker.h"

worker_mutex_t mutex;
worker_mutex_t mutex1;
worker_mutex_t mutex2;
int inc = 0;
int inc1 = 0;
int inc2 = 0;

void *func(void *args){

	for(int i = 0; i < 10000000; i++){
		worker_mutex_lock(&mutex);
		inc++;
		worker_mutex_unlock(&mutex);
		worker_mutex_lock(&mutex1);
		inc1++;
		worker_mutex_unlock(&mutex1);
		worker_mutex_lock(&mutex2);
		inc2++;
		worker_mutex_unlock(&mutex2);
	}
	
	worker_exit(NULL);
}

void *b_func(void *args){
	for(int i = 0; i < 1000000000; i++){

	}

	worker_exit(NULL);
}

void *func1(void *args){
	worker_t wt[2];
	for(int i = 0; i < 2; i++){
		worker_create(&wt[i], NULL, &b_func, &wt[i]);
	}

	for(int i = 0; i < 2; i++){
		worker_join(wt[i], NULL);
	}

	// printf("Thread %d Finished\n", *(int *)args);
	worker_exit(NULL);
}

/* A scratch program template on which to call and
 * test thread-worker library functions as you implement
 * them.
 *
 * You can modify and use this program as much as possible.
 * This will not be graded.
 */
int main(int argc, char **argv) {

	/* Implement HERE */

	worker_t wt[256];

	for(int i = 0; i < 256; i++){
		worker_create(&wt[i], NULL, &b_func, &wt[i]);
	}

	for(int i = 0; i < 256; i++){
		worker_join(wt[i], NULL);
	}

	for(int i = 0; i < 256; i++){
		worker_create(&wt[i], NULL, &b_func, &wt[i]);
	}

	for(int i = 0; i < 256; i++){
		worker_join(wt[i], NULL);
	}

	// for(int i = 0; i < 10; i++){
	// 	worker_create(&wt[i], NULL, &func1, &wt[i]);
	// }

	// for(int i = 0; i < 10; i++){
	// 	worker_join(wt[i], NULL);
	// }

	// MUTEX tests (main function)
	// printf("%d\n", SIGSTKSZ);
	// worker_mutex_init(&mutex, NULL);
	// worker_mutex_init(&mutex1, NULL);
	// worker_mutex_init(&mutex2, NULL);
	
	// for(int i = 0; i < 5; i++){
	// 	worker_create(&wt[i], NULL, &func, &wt[i]);
	// }

	// for(int i = 0; i < 5; i++){
	// 	worker_join(wt[i], NULL);
	// }

	// worker_mutex_destroy(&mutex);
	// worker_mutex_destroy(&mutex1);
	// worker_mutex_destroy(&mutex2);
	// printf("inc: %d\n", inc);
	// printf("inc1: %d\n", inc1);
	// printf("inc2: %d\n", inc2);

	return 0;
}
