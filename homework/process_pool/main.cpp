#include "process_pool.hpp"
#include <time.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

void* print(void* arg, ProcessPool* pool){
    printf("Hello World:).\n");
    return NULL;
}

int is_free(){
    int file_desc;
    if ((file_desc = open("pid", O_WRONLY | O_TRUNC | O_CREAT)) < 0){
        perror("open lock file failed.\n");
        return 0;
    }
    
    struct flock lock;
    lock.l_type = F_WRLCK;
    lock.l_start = 0;
    lock.l_whence = SEEK_SET;
    lock.l_len = 0;
    
    if(fcntl(file_desc, F_SETLK, &lock) == -1){
        int err = errno;
        
        perror("FCNTL has failed.");
        
        if (err == EAGAIN){
            perror("Another Socket is runnig now.");
        }
        
        return 0;
    }
    
    return 1;
}
int main(int argc, const char * argv[]) {
    if(argc != 2){
        printf("Usage: ./xxx port(port stands for the port wanna listen on.)\n");
        exit(1);
    }
    
    signal(SIGCHLD, SIG_IGN);
    
    if(!is_free()){
//        exit(0);
    }
    
    int port = atoi(argv[1]);
    
    struct sockaddr_in addr;
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);
    
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    bind(server_socket, (struct sockaddr *) &addr, sizeof(addr));
    listen(server_socket, 0);
    
    ProcessPool* pool = new ProcessPool(server_socket, 1, 10);
    pool->init();
    pool->addTask(print, NULL);
    pool->start();

    if(pool)
        delete(pool);
    exit(0);
}
