#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include "../thread-worker.h"

void *func(void *args){

	for(int i = 0; i < 10; i++){
		printf("executing\n");
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
	worker_t wt = 1;
	worker_create(&wt, NULL, &func, NULL);
	worker_join(wt, NULL);

	return 0;
}
