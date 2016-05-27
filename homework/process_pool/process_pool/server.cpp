#include <time.h>
#include <sys/stat.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include "process_pool.hpp"

void* print(void* arg, ProcessPool* pool){
    pid_t pid = getpid();
    printf("Hello World from process: %d.\n", pid);
    return NULL;
}

void* echo(void* arg){
    return arg;
}

void* answer(void* arg, ProcessPool* pool){
    pid_t pid = getpid();
    printf("Hello World from process: %d.\n", pid);

    task_t* task = (task_t*) arg;
    void* rst;
    
    printf("Welcome to join the new world, %i\n", pid);
    
    message_t* msg;
    char buff[BUFFER_SIZE];

    int connect_fd;
    int clnt_sock;
    while(!pool->isClosed){
        printf("Hi from the child.\n");
        pool->wait_sem(1);
        pool->wait_sem(0);
        msg = pool->dequeue();
        connect_fd = msg->connect_fd;

//        char *dst = buff;
//        char *src = msg->input;
//        while(*src){
//            *dst++ = *src++;
//        }
        pool->signal_sem(0);
        pool->signal_sem(2);
        sleep(2);
        clnt_sock = dup(connect_fd);
        close(connect_fd);
//
        read(clnt_sock, buff, BUFFER_SIZE);
        printf("Process %i got input data %s\n", pid, buff);
        printf("The File Descriptor: %i\n", connect_fd);
        rst = task->function(buff);
        char r[] = "hello clinet I love you from the sub-process\n";
        write(connect_fd, r, sizeof(r));
//        write(connect_fd, buff, strlen(buff));
        close(clnt_sock);
        printf("Already delete the socket: %i\n", connect_fd);
    }
//
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

    int port = atoi(argv[1]);

    printf("Running server on port: %d and process: %d\n", port, getpid());

    int serv_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    
    //将套接字和IP、端口绑定
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;  //使用IPv4地址
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");  //具体的IP地址
    serv_addr.sin_port = htons(port);  //端口
    bind(serv_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    
    //进入监听状态，等待用户发起请求
    listen(serv_sock, 20);
    
    //接收客户端请求
//    struct sockaddr_in clnt_addr;
//    socklen_t clnt_addr_size = sizeof(clnt_addr);
//    
//    
//    int clnt_sock;
//    while((clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_addr, &clnt_addr_size))){
//        printf("%d\n", clnt_sock);
//        
//        
//        //    //向客户端发送数据
//        char str[] = "Hello World!";
//        write(clnt_sock, str, sizeof(str));
//        break;
//
//    }
//
//    //关闭套接字
//    close(clnt_sock);
//    close(serv_sock);
    
//
//    struct sockaddr_in addr;
//    memset(&addr, 0, sizeof(addr));
//    addr.sin_family = AF_INET;
//    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
//    addr.sin_port = htons(port);
//    
//    int server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
//    bind(server_socket, (struct sockaddr *) &addr, sizeof(addr));
//    listen(server_socket, 20);
//    
//    struct sockaddr_in client_addr;
//    int connect_fd = accept(server_socket, (struct sockaddr*)&client_addr, (socklen_t *)sizeof(client_addr));
//    //向客户端发送数据
//    char str[] = "Hello World!";
//    write(connect_fd, str, sizeof(str));
//    
//    //关闭套接字
//    close(connect_fd);
//    close(server_socket);
//    char buffer[BUFFER_SIZE];
//    read(connect_fd, buffer, sizeof(buffer)-1);
//    printf("Message form server: %s\n", buffer);
//    write(connect_fd, buffer, sizeof(buffer));
    
    task_t* task = (task_t*) malloc(sizeof(task_t));
    task->arg = NULL;
    task->function = echo;
    
    ProcessPool* pool = new ProcessPool(serv_sock, 1, 2);
    pool->init();
    while(pool->addTask(answer, task));
    pool->start();
    
    int clnt_sock;
    int clnt_sock_2;
    struct sockaddr_in clnt_addr;
    socklen_t clnt_addr_size = sizeof(clnt_addr);
    
    char buff[BUFFER_SIZE];
    long count = 0;
    message_t* msg;
    
    while(!pool->isClosed){
        printf("Waiting for clients...\n");
        clnt_sock = accept(pool->get_socket_desc(), (struct sockaddr*)&clnt_addr, &clnt_addr_size);
        
        
        printf("Recv the request. Descritor: %d\n", clnt_sock);
//        count = read(clnt_sock, buff, BUFFER_SIZE);
        
//        char r[] = "hello clinet I love you from the server\n";
//        write(clnt_sock, r, sizeof(r));
        clnt_sock_2 = dup(clnt_sock);
        close(clnt_sock);
        
        pool->wait_sem(2);
        pool->wait_sem(0);
        
        msg = pool->enqueue();
        msg->connect_fd = clnt_sock_2;
//        char *dst = msg->input;
//        char *src = buff;
//        while(*src){
//            *dst++ = *src++;
//        }
        
//        printf("Got request at: %d\n", clnt_sock_2);
//        printf("Got input content: %s\n", msg->input);
        
        pool->signal_sem(1);
        sleep(2);
        pool->signal_sem(0);
    }

//
//    if(pool)
//        delete(pool);
//    exit(0);
}
