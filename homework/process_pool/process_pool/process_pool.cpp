#include "process_pool.hpp"
#include <signal.h>

const int MODE = 0666;


ProcessPool::ProcessPool(const int& socket, const int& size, const int& max_queue_s)
    :socket_desc(socket),
    max_size(size),
    max_queue_size(max_queue_s + 1),
    shm_id(-1),
    semset_id(-1),
    isClosed(false),
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
    this->ptr[0].head = 1;
    this->ptr[0].tail = 0;
    
//    printf("%ld, %ld, %ld, %ld.\n", sizeof(position_t), sizeof(message_t), sizeof(process_t), shm_size);
    
    // Init the message quene in the share memory.
    for(int i = 0; i < this->max_queue_size; i++){
        this->msg_ptr[i].connect_fd = -i;
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
    sem.val = 1;
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
    sem.val = this->max_queue_size - 1;
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
//                kill(0, SIGSTOP);
                this->pro_ptr[i].pid = getpid();
                printf("Getting Start at %i\n", pid);
                this->pro_ptr[i].function(this->pro_ptr[i].arg, this);
                exit(0);
                break;
            }
        }
    }
    
    return 0;
}

void ProcessPool:: wait_sem(int sem_num){
    struct sembuf buf;
    buf.sem_num = sem_num;
    buf.sem_op = -1;
    buf.sem_flg = SEM_UNDO;
    if(semop(this->semset_id, &buf, 1) < 0){
        perror("Error while semaphore waiting.");
    }
}

void ProcessPool:: signal_sem(int sem_num){
    struct sembuf buf;
    buf.sem_num = sem_num;
    buf.sem_op = 1;
    buf.sem_flg = SEM_UNDO;
    if(semop(this->semset_id, &buf, 1) < 0){
        perror("Error while semaphore signaling.");
    }
}

message_t* ProcessPool::dequeue(){
    message_t *msg;
    int length = ((this->ptr->tail + this->max_queue_size) - this->ptr->head + 1) % this->get_max_queue_size();
    if(length == 0){
        return NULL;
    }
    
    msg = this->msg_ptr + this->ptr->head;
    
    this->ptr->head = (this->ptr->head + 1) % this->max_queue_size;
    
    return msg;
}

message_t* ProcessPool::enqueue(){
    message_t* msg;
    if(((this->ptr->tail + 2) % this->max_queue_size) == this->ptr->head)
        return NULL;
    
    this->ptr->tail = (this->ptr->tail + 1) % this->max_queue_size;
    msg = this->msg_ptr + this->ptr->tail;
    return msg;
}
