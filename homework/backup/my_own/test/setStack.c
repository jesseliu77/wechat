#include <pthread.h>
#include <stdio.h>
#define N 10000
#define EXTRA 1000000
pthread_attr_t attr;

void *dowork(void *t){
	double A[N][N];
	size_t size;
	pthread_attr_getstacksize(&attr, &size);
	printf("The stack size inside the thread: %li\n", size);
	pthread_exit(NULL);
	return NULL;
}

int main(int argc, char *argv[]){
	pthread_attr_init(&attr);
	size_t size;
	int rc;
	pthread_attr_getstacksize(&attr, &size);
	printf("Default stack size: %li\n", size);

	size = 16 * N * N;
	size = sizeof(double) * N * N + EXTRA;
	printf("Trying to set the stack size of %li\n", size);

	rc = pthread_attr_setstacksize(&attr, size);

	if(rc){
		printf("Fail to set the stack size of %li\n", size);
		printf("Error code: %d\n", rc);
	} else{
		printf("Success! Síet the stack size of %li\n", size);
	}
	pthread_t thread;
	pthread_create(&thread, &attr, dowork, NULL);

	pthread_join(thread, NULL);
	pthread_exit(NULL);
	return 0;
}