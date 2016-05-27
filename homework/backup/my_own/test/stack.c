#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#define NTHREADS 4
#define N 1000
#define MEGEXTRA 1000000

pthread_attr_t attr;

void *threadentry(void *p){
	double A[N][N];
	long tid = (long)p;
	printf("Hellow I'm Thread %ld.\n", tid);
	for(int i = 0; i < N; i++)
		for(int j = 0; j < N; j++){
			A[i][j] = ((i+j)/3.452) + (N - i);
			A[i][j] = (i+j)/3.42;
		}
	printf("Goodbye I'm Thread %ld. \n", tid);
	pthread_exit(NULL);
	return NULL;
}
void *dowork(void *threadid)
{
	double A[N][N];
	int i, j;
	long tid;

	size_t mystacksize;

	tid = (long)threadid;
	
	printf("Hello I'm Thread %ld. \n", tid);
	pthread_attr_init(&attr);
	pthread_attr_getstacksize(&attr, &mystacksize);
	printf("Thread %ld: stack size = %li bytes \n", tid, mystacksize);
	// for (int i=0; i<N; i++)
	// 	for (int j=0; j<N; j++)
	// 		A[i][j] = ((i+j)/3.452) + (N-i);
	
	printf("Something wrong? Nope.\n");
	printf("Goodbye I'm Thread %ld. \n", tid);
	pthread_exit(NULL);
	return NULL;
}

int main(int argc, char *argv[])
{
	pthread_t threads[NTHREADS];
	size_t stacksize;

	int rc;
	long t;

	// pthread_attr_init(&attr);
	// pthread_attr_getstacksize(&attr, &stacksize);
	// printf("Default stack size: %li bytes \n", stacksize);

	// stacksize = 16 * N * N;
	// printf("Amount that needed: %li bytes \n", stacksize);
	// pthread_attr_setstacksize(&attr, stacksize);
	
	pthread_attr_init(&attr);
	pthread_attr_getstacksize (&attr, &stacksize);
	printf("Default stack size = %li\n", stacksize);
	stacksize = 64*N*N;
	printf("Amount of stack needed per thread = %li\n",stacksize);
	pthread_attr_setstacksize (&attr, stacksize);
	printf("Creating threads with stack size = %li bytes\n",stacksize);

	for (t=0; t<NTHREADS; t++){
		printf("Enter with %ld. \n", t);
		rc = pthread_create(&threads[t], &attr, dowork, (void *) t);
		printf("Return code: %d. \n", rc);
		if (rc) {
			printf("ERROR. Retrun code: %d \n", rc);
			exit(-1);
		}
	}
	printf("Created %ld threads. \n", t);
	pthread_exit(NULL);

}
