#include "thread_pool.hpp"

Worker::Worker(void *(*function)(void *), void *arg, const int& priority)
:function(function),
arg(arg),
priority(priority){
    this->task_id = Worker::current_id++;
}

Worker *Worker_Queue::next(){
    Worker* worker;
    Thread_Pool* pool = (Thread_Pool *)this->get_pool();
    
    pthread_mutex_lock(&(pool->mutex));
    
    if(pool->get_status() == CLOSED){
        if(this->length != 0){
            worker = this->head;
            this->head = this->head->next;
            this->length -= 1;
            
            pthread_mutex_unlock(&(pool->mutex));
        }
        return worker;
    }
    
    if(pool->get_status() == SUSPEND){
        pthread_mutex_unlock(&(pool->mutex));
        printf("The Pool is dead.\n");
        return NULL;

    }
    
    while(this->length == 0){
        printf("The queue is empty. Waiting...\n");
        pthread_cond_wait(&(pool->not_empty), &(pool->mutex));
    }
    
    worker = this->head;
    this->head = this->head->next;
    this->length -= 1;
    if(this->length == 0){
        this->tail = NULL;
        pthread_cond_broadcast(&(pool->is_empty));
    }
    
    pthread_cond_broadcast(&(pool->not_full));
    
    pthread_mutex_unlock(&(pool->mutex));
    return worker;
}

Worker* Worker_Queue::add(int priority){
    Worker* worker;
    
    Thread_Pool* pool = (Thread_Pool*)this->get_pool();
    
    pthread_mutex_lock(&(pool->mutex));
    
    if(pool->get_status() == CLOSED){
        printf("The thread pool is closed.\n");
        pthread_mutex_unlock(&(pool->mutex));
        return worker;
    }
    
    if(pool->get_status() == SUSPEND){
        printf("The thread pool is suspend.\n");
        pthread_mutex_unlock(&(pool->mutex));
        return worker;
    }
    
    while(this->length == this->size){
        printf("The worker queue is full. Waiting...\n");
        pthread_cond_wait(&(pool->not_full), &(pool->mutex));
    }
    
    worker = (Worker*)malloc(sizeof(Worker));
    
    if(this->head == NULL){
        this->head = worker;
        this->tail = worker;
        pthread_cond_broadcast(&(pool->not_empty));
    }else if(priority <= this->tail->priority){
        worker->next = this->tail;
        this->tail = worker;
    }else if(priority > this->head->priority){
        this->head->next = worker;
        this->head = worker;
        return worker;
    } else{
        Worker *p = this->tail->next, *q = this->tail;
        while(p && q && priority >= p->priority){p++;q++;}
        
        worker->next = p;
        q->next = worker;
    }
    
    
    pthread_mutex_unlock(&(pool->mutex));
    return worker;
}

void Worker_Queue::add(Worker* worker){
    Worker* pos = this->add(worker->priority);
    
    if(pos){
        pos->arg = worker->arg;
        pos->function = worker->function;
        pos->priority = worker->priority;
        pos->task_id = worker->task_id;
    }
    
    return;
}

Thread_Pool::Thread_Pool(const int& thread_num, const int& size, Worker_Container container)
:thread_num(thread_num),
size(size),
status(SUSPEND),
container(container){
    
    pthread_attr_init(&this->attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    
    if (pthread_mutex_init(&(this->mutex), NULL)){
        printf("Fail to init the pool mutex.\n");
        exit(1);
    }
    
    if(pthread_cond_init(&(this->is_empty), NULL)){
        printf("Fail to init the pool condition variable.\n");
        printf("Condition Variable: Queue_Empty Condition.\n");
        exit(1);
    }
    
    if(pthread_cond_init(&(this->not_empty), NULL)){
        printf("Fail to init the pool condition variable.\n");
        printf("Condition Variable: Queue_Not_Empty Condition.\n");
        exit(1);
    }
    
    if(pthread_cond_init(&(this->not_full), NULL)){
        printf("Fail to init the pool condition variable.\n");
        printf("Condition Variable: Queue_Not_Full Condition.\n");
        exit(1);
    }
    
    Thread* threads = (Thread *)malloc(sizeof(Thread) * this->thread_num);
    
    this->threads = threads;
    pthread_t pthread;
    
    int rc;
    for(int i = 0; i < this->thread_num; i++){
        if((rc = pthread_create(&pthread, &attr, Thread_Pool::static_thread_function, (void *)this)) < 0){
            perror("Creat Thread Error Occur.\n");
            exit(1);
        }
        
        threads->thread = pthread;
        threads->id = i;
        threads++;
    }
}

int Thread_Pool::start(){
    this->status = RUNNING;
    return 0;
}

int Thread_Pool::pause(){
    this->status = SUSPEND;
    return 0;
}

int Thread_Pool::resume(){
    return this->start();
}
