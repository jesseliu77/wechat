#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

typedef struct data data;
struct data{
    int value;
    int id;
};

#define size 4106
#define mode 0666

int main2(int argc, const char * argv[]){
    int shm_id;
    data *ptr, *shmptr;
    ptr = (data *)malloc(sizeof(data) * size);
    for(int i = 0; i < size; i++){
        ptr[i].value = i;
        ptr[i].id = i * 10;
    }
    
    shm_id = shmget(IPC_PRIVATE, sizeof(data) * size, mode);
    //set the shm;
    shmptr = shmat(shm_id, 0, 0);
    for(int i = 0; i < size; i++){
        shmptr[i] = ptr[i];
    }
    
    shmdt(shmptr);
    printf("Parent Process: %d.\n", getpid());
    pid_t pid = fork();
    if(pid == 0){
        printf("Child Process: %d.\n", getpid());
        //get the shm
        shmget(shm_id, 0, 0);
        struct shmid_ds *buf;
        shmctl(shm_id, IPC_STAT, buf);
        printf("size: %ld", buf->shm_segsz);
        printf("creator: %d", buf->shm_cpid);
        printf("num attach: %hu", buf->shm_nattch);
        printf("lpid: %d", buf->shm_lpid);
        data *newptr = shmat(shm_id, 0, 0);
        for(int i = 0; i < size; i++){
            printf("Value: %d, ID: %d, i: %d.\n", newptr[i].value, newptr[i].id, i);
        }
        exit(0);
    }
    return 0;
}
