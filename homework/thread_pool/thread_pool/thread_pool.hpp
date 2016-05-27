#ifndef thread_pool_hpp
#define thread_pool_hpp

#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>

#define LOW 0
#define DEFAULT 1
#define MIDIUM 2
#define HIGH 3

#define IDLE 0
#define BUSY 1

#define CLOSED 0
#define RUNNING 1
#define SUSPEND 2

class Thread{
public:
    int id;
    pthread_t thread;
};

class Worker{
private:
    long static current_id;
public:
    Worker(void *(*function)(void *), void *arg, const int& priority = DEFAULT);
    long task_id;
    void *(*function)(void *);
    void *arg;
    int priority;
    Worker* next;
};


class Worker_Container{
private:
    void* pool;

public:
    virtual Worker* next();
    void* get_pool();
};



class Worker_Queue: Worker_Container{
private:
    int size;
    int length;
    Worker* head;
    Worker* tail;
    Worker* add(int priority);
public:
    Worker_Queue(const int &size):
    size(size),
    length(0),
    head(NULL),
    tail(NULL){
    };
    
    Worker* next();
    void add(Worker* worker);
};


class Thread_Pool{
public:
    Thread_Pool(const int& thread_num, const int& size, Worker_Container container);
    int start();
    int destroy();
    int pause();
    int resume();
    int expand();
    
    virtual void* thread_function(void *arg);
    static void *static_thread_function(void *arg){
        void* result = static_cast<Thread_Pool*>(arg)->thread_function(arg);
        return result;
    }
    int get_status();
    pthread_attr_t attr;
    pthread_cond_t is_empty;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
    pthread_mutex_t mutex;

private:
    int status;
    int thread_num;
    int size;
    Worker_Container container;

    void* result;
    Thread* threads;
    
};

class Echo_Thread_Pool:Thread_Pool{
private:
    int socket;
    
    pthread_t request_thread;
    
public:
    
};

#endif
