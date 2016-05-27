#include<stdio.h>

void say(){
	printf("hello world.\n");
}
int main(){
	pthread_t first_thread;
	pthread_create(&first_thread, NULL, (void *)say, NULL);
	pthread_join(first_thread, NULL);
	

}
