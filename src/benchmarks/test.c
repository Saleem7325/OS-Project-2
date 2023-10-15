#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include "../thread-worker.h"

void *func(void *args){

	for(int i = 0; i < 2; i++){
		printf("executing\n");
		worker_yield();
	}

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
	worker_t wt[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9 };
	
	for(int i = 0; i < 9; i++){
		worker_create(&wt[i], NULL, &func, NULL);
	}

	for(int i = 0; i < 9; i++){
		worker_join(wt[i], NULL);
	}


	return 0;
}
