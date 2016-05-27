#ifndef process_pool_hpp
#define process_pool_hpp

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

typedef struct process_t process_t;
typedef struct message_t message_t;
typedef struct position_t position_t;

const long BUFFER_SIZE = 4096;

class ProcessPool{
public:
    ProcessPool(const int& socket, const int& size, const int& max_queue_s);
    ~ProcessPool();
    int start();
    void init();
    int addTask(void *(*function)(void *, ProcessPool*), void* arg);
    int get_socket_desc(){
        return this->socket_desc;
    };
    int get_max_size(){
        return this->max_size;
    };
    int get_max_queue_size(){
        return this->max_queue_size;
    }
    int get_shm_id(){
        return this->shm_id;
    }
    int get_semset_id(){
        return this->semset_id;
    }
    process_t* get_position(){
        return this->pro_ptr;
    }
    message_t* get_msg_ptr(){
        return this->msg_ptr;
    }
    process_t* get_pro_ptr(){
        return this->pro_ptr;
    }
    
    void wait_sem(int sem_num);
    void signal_sem(int sem_num);
    message_t* dequeue();
    bool isClosed;
    message_t* enqueue();


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

/**
 * Just a encapsulation of process to indicate weather the process is idle or not;
 */
struct process_t{
    pid_t pid;
    bool is_alive;
    bool is_busy;
    void *(*function)(void *, ProcessPool *);
    void *arg;
    
};

/**
 * After the processing, the result should be stored in the message struct.
 */
struct message_t{
//    char input[BUFFER_SIZE];
    
    int connect_fd;
};

struct position_t{
    int head;
    int tail;
};

/**
 * Basically, the strut 并没有什么卵用.
 */
struct task_t{
    /**
     * The specific task;
     **/
    void *(*function)(void *arg);
    
    /**
     * The arguments for the call back function;
     **/
    void *arg;
};
#endif