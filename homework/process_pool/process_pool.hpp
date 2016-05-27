#ifndef process_pool_hpp
#define process_pool_hpp

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/sem.h>

typedef struct process_t process_t;
typedef struct message_t message_t;
typedef struct position_t position_t;

class ProcessPool{
public:
    ProcessPool(const int& socket, const int& size, const int& max_queue_s);
    ~ProcessPool();
    int start();
    void init();
    int addTask(void *(*function)(void *, ProcessPool*), void* arg);
private:
    int socket_desc;
    int shm_id;
    int semset_id;
    position_t* ptr;
    message_t* msg_ptr;
    process_t* pro_ptr;
    int max_size;
    int max_queue_size;
};
#endif