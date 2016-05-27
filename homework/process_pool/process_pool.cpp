#include "process_pool.hpp"


const int MODE = 0666;
const int BUFFER_SIZE = 1024;
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
    char input[BUFFER_SIZE];
    char output[BUFFER_SIZE];
    int child_pid;
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

ProcessPool::ProcessPool(const int& socket, const int& size, const int& max_queue_s)
    :socket_desc(socket),
    max_size(size),
    max_queue_size(max_queue_s + 1),
    shm_id(-1),
    semset_id(-1),
    ptr((position_t *) -1),
    msg_ptr((message_t *)-1),
    pro_ptr((process_t *)-1)
    {}

ProcessPool::~ProcessPool(){
    if(this->shm_id >= 0){
        shmctl(this->shm_id, IPC_RMID, NULL);
    }
}
void ProcessPool::init(){
    //First init the share memory;
    size_t shm_size = sizeof(position_t) + sizeof(message_t) * this->max_queue_size + sizeof(process_t) * this->max_size;
    this->shm_id = shmget(IPC_PRIVATE, shm_size, MODE);
    if(this->shm_id < 0){
        perror("Error Occured While Creating Share Memory.");
        printf("Asked Memory Size: %ld.\n", shm_size);
        delete(this);
        exit(1);
    }
    
    this->ptr = (position_t *)shmat(this->shm_id, 0, 0);
    if(this->ptr == (position_t *) -1){
        perror("Error Occured While Connecting the shared Memory.");
        delete(this);
        exit(1);
    }
    
    this->msg_ptr = (message_t *)(this->ptr+1);
    this->pro_ptr = (process_t *)(this->msg_ptr+this->max_queue_size);
    
    // Init the position in the share memory.
    this->ptr[0].head = 0;
    this->ptr[0].tail = 0;
    
    printf("%ld, %ld, %ld, %ld.\n", sizeof(position_t), sizeof(message_t), sizeof(process_t), shm_size);
    
    // Init the message quene in the share memory.
    for(int i = 0; i < this->max_queue_size; i++){
        this->msg_ptr[i].child_pid = -i;
    }
    
    
    // Init the process in the share memory.
    for(int i = 0; i < this->max_size; i++){
        this->pro_ptr[i].is_alive = false;
        this->pro_ptr[i].is_busy = false;
        this->pro_ptr[i].pid = 0;
    }
    
    /** ============================================================ */
    
    
    /** 
     * And then init the semaphore.
     * Note the index for the 3 semaphore is: [0, 1, 2] = [is_occupy, is_empty, is_full]
    */
    this->semset_id = semget(IPC_PRIVATE, 3, MODE);
    if(this->semset_id < 0){
        perror("Error Occured While Creating Semaphore.");
        exit(1);
    }
    union semun sem;
    // Init occupy to zero. where 1 stands for occupied.
    sem.val = 0;
    if(semctl(this->semset_id, 0, SETVAL, sem) < 0){
        perror("semclt Error.");
        delete(this);
        exit(1);
    }
    sem.val = 0;
    if(semctl(this->semset_id, 1, SETVAL, sem) < 0){
        perror("semclt Error.");
        exit(1);
    }
    sem.val = this->max_queue_size;
    if(semctl(this->semset_id, 2, SETVAL, sem) < 0){
        perror("semclt Error.");
        delete(this);
        exit(1);
    }

}

int ProcessPool::addTask(void *(*function)(void *, ProcessPool* ), void* arg){
    for(int i = 0; i < this->max_size; i++){
        if (this->pro_ptr[i].is_alive ){
            continue;
        }
        this->pro_ptr[i].is_alive = true;
        this->pro_ptr[i].function = function;
        this->pro_ptr[i].arg = arg;
        return 1;
    }
    return 0;
}

int ProcessPool::start(){
    //fork some processes;
    for(int i = 0; i < this->max_size; i++){
        pid_t pid;
        if (this->pro_ptr[i].is_alive){
            pid = fork();
            if(pid == 0){
                this->pro_ptr[i].pid = getpid();
                void* result = this->pro_ptr[i].function(this->pro_ptr[i].arg, this);
                exit(0);
                break;
            }
        }
    }
    return 0;
}

