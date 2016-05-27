#include "thread_pool.h"
#include <assert.h>
struct thread_task_t{
	/**
	  * The specific task;
	**/
	void *(*function)(void *arg);

	/**
	  * The arguments for the call back function;
	**/
	void *arg;

	/**
	  * The pointer points to the next task;
	**/
	struct thread_task_t *next;
};

struct thread_pool_t{
	/**
	  * The total thread number;
	**/
	int thread_num;

	/**
	  * The max tasks number in the task queue.
	  * Note that the thread pool will refuse to add a new task if the current queue size reach the limit;
	**/
	int max_queue_num;

	/**
	  * The head and the tail of the work queue;
	**/
    thread_task_t *head;
    thread_task_t *tail;

	/**
	  * The pthread objects in the thread_pool;
	**/
	pthread_t *pthreads;

	/**
	  * The mutex object. If a thread in the pool wanna operate on the parameters, like queue_size, it must lock the mutex;
	**/
	pthread_mutex_t mutex;

	/**
	  * The condition variable that indicates weather the queue is empty;
	**/
	pthread_cond_t queue_empty;

	/**
	  * The condition variable that indicates weather the queue is not empty;
	**/
	pthread_cond_t queue_not_empty;

	/**
	  * The condition variable that indicates weather the queue is not full;
	**/
	pthread_cond_t queue_not_full;

	/**
	  * The curruent queue size in the pool;
	**/
	int queue_size;
	
	/**
	  * Weather the pool is active or not;
	**/
	int isClosed;
};

/**
 * @function: thread_pool_create
 * @desc: Creates a thread_pool_t object.
 * @param thread_num: thread num
 * @param max_queue_num: max task queue size
 * @return: a newly created thread pool or NULL
 */
thread_pool_t *thread_pool_create(int thread_num, int max_queue_num){
	thread_pool_t *pool = (thread_pool_t *)malloc(sizeof(thread_pool_t));
	pool->thread_num = thread_num;
	pool->max_queue_num = max_queue_num;
	
    /**
      * Actually, we don't need to init the task array here. Since we use link instead of array to simulate the queue.
	  * pool->head = (struct thread_task_t *)malloc(sizeof(struct thread_task_t) * max_queue_num);
	  * pool->tail = pool->head;
    **/
    pool->head = NULL;
    pool->tail = NULL;
    
    pool->queue_size = 0;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	
	pthread_t *pthreads = (pthread_t *)malloc(sizeof(pthread_t) * thread_num);
    pool->pthreads = pthreads;
    pool->isClosed = 0;
	for(int i = 0; i < thread_num; i++){
		int rc = pthread_create(pthreads++, &attr, thread_function, (void *) pool);
		if(rc){
			printf("Error while creating Thread %d. Exiting.\n", i);
			return NULL;
		}
	}
    pthread_attr_destroy(&attr);
    
	if (pthread_mutex_init(&(pool->mutex), NULL)){
		printf("Fail to init the pool mutex.\n");
		return NULL;
	}

	if(pthread_cond_init(&(pool->queue_empty), NULL)){
		printf("Fail to init the pool condition variable.\n");
		printf("Condition Variable: Queue_Empty Condition.\n");
        return NULL;
	}

	if(pthread_cond_init(&(pool->queue_not_empty), NULL)){
		printf("Fail to init the pool condition variable.\n");
		printf("Condition Variable: Queue_Not_Empty Condition.\n");
        return NULL;
	}

	if(pthread_cond_init(&(pool->queue_not_full), NULL)){
		printf("Fail to init the pool condition variable.\n");
		printf("Condition Variable: Queue_Not_Full Condition.\n");
        return NULL;
	}
	/**
	if(pthread_cond_init(&(pool->queue_empty), NULL)){
		printf("Fail to init the pool condition variable.\n");
		printf("Condition Variable: Queue Empty Condition.\n");
		return NULL:
	}
	**/

	return pool;

};

/**
  * @function: thread_pool_add.
  * @desc: Add a task to the specific thread pool.
  * @param pool: The thread_pool_t object.
  * @param function: The detail of the task.
  * @param arg: The arguments of the task.
  * @return: 0 stands for success, others stands for fail to add the task.
  */
int thread_pool_add(thread_pool_t *pool, void *(*function)(void *arg), void *arg){
    assert(pool != NULL);
    assert(function != NULL);
    
    // First try to gain the mutex lock
    pthread_mutex_lock(&(pool->mutex));
    
    //Case 1: The pool is closed right now;
    if(pool->isClosed){
        pthread_mutex_unlock(&(pool->mutex));
        printf("The Pool is closed!\n");
        return 1;
    }
    
    //case 2.1: The pool is full. Wait until the pool is not full.
    while(pool->queue_size == pool->max_queue_num){
        printf("The Task Queue is full right now:).\n");
        pthread_cond_wait(&(pool->queue_not_full), &(pool->mutex));
    }
    
    //case 2.2: The pool is not full, try to add the task to the queue;
    thread_task_t *task = (thread_task_t *) malloc(sizeof(thread_task_t));
    
    //case 2.2.1: Fail to malloc the memory to the task.
    if (task == NULL){
        printf("Fail to allocate the memory!\n");
        pthread_mutex_unlock(&(pool->mutex));
        return 221;
    }
    
    //case 2.2.2: add the task to the queue.
    task->function = function;
    task->arg = arg;
    task->next = NULL;
    
    //case 2.2.2.1: the queue is empty.
    if(pool->head == NULL){
        pool->head = task;
        pool->tail = task;
        // Tell the threads that the queue is not longer empty.
        pthread_cond_broadcast(&(pool->queue_not_empty));
    }
    //case 2.2.2.2: the queue is not empty. Just add the task to the tail.
    else{
        pool->tail->next = task;
        pool->tail = task;
    }
    
    pool->queue_size += 1;
    pthread_mutex_unlock(&(pool->mutex));
    return 0;
}

void *thread_function(void *arg){
    assert(arg != NULL);
    
    thread_pool_t *pool = (thread_pool_t *)arg;
    thread_task_t *task = NULL;
    
    for(;;){
        // Try to lock the mutex;
        pthread_mutex_lock(&(pool->mutex));
        
        // case 2.1: Wait until the queue is not empty;
        while(pool->queue_size == 0 && !pool->isClosed){
            pthread_cond_wait(&(pool->queue_not_empty), &(pool->mutex));
        }
        
        // Case 1: if the pool is closed, just exit the current thread.
        if(pool->isClosed){
            printf("Good Bye. It's my time to go. It's just my time. \nI will always miss you, my dearest Darling:).\n");
            pthread_mutex_unlock(&(pool->mutex));
            printf("Unlocked successfullly.\n");
            pthread_exit(NULL);
        }

        // case 2.2: when the queue is not empty.
        task = pool->head;
        pool->head = task->next;
        pool->queue_size -= 1;
        
        // Tell the destroy function, the queue is empty right now.
        if(pool->queue_size == 0){
            pool->tail = NULL;
            pthread_cond_signal(&(pool->queue_empty));
        }
        
        // If the queue just turns to un-full, signal the thread_pool_add function.
        if(pool->queue_size == pool->max_queue_num - 1){
            pthread_cond_signal(&(pool->queue_not_full));
        }
        
        // Unlock the mutex
        pthread_mutex_unlock(&(pool->mutex));
        task->function(task->arg);
        free(task);
        task = NULL;
    };
};

int thread_pool_destroy(thread_pool_t *pool){
    assert(pool != NULL);
    
    pthread_mutex_lock(&(pool->mutex));
    
    if(pool->isClosed){
        printf("The thread pool had closed!\n");
        return -1;
    }
    
    while(pool->queue_size != 0){
        pthread_cond_wait(&(pool->queue_empty), &(pool->mutex));
    }
    
    pool->isClosed = 1;
    pthread_mutex_unlock(&(pool->mutex));
    pthread_cond_broadcast(&(pool->queue_not_empty));
    pthread_cond_broadcast(&(pool->queue_not_full));
    
    for(int i = 0; i < pool->thread_num; i++){
        printf("waiting Thread-%d to exit...\n", i);
        pthread_join(pool->pthreads[i], NULL);
        printf("Thread-%d exit successfully.\n", i);
    }
    
//    printf("Now let's clean up the stack and heap:).\n");
    // Release all the resources;
    pthread_mutex_destroy(&(pool->mutex));
    pthread_cond_destroy(&(pool->queue_empty));
    pthread_cond_destroy(&(pool->queue_not_full));
    pthread_cond_destroy(&(pool->queue_not_empty));
    free(pool->pthreads);
    
    thread_task_t *task;
    while(pool->head){
        task = pool->head;
        pool->head = pool->head->next;
        free(task);
    }
    free(pool);
    
    printf("Successfully free the resouces.\n");
    return 0;
}