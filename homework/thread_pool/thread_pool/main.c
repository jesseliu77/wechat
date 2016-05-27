//
//  main.c
//  thread_pool
//
//  Created by LiuWenjie on 5/15/16.
//  Copyright © 2016 Jesse. All rights reserved.
//
#include "thread_pool.h"
#include <unistd.h>
void *print(void *arg){
    sleep(1);
    printf("Hello World: %d\n", (int) arg);
    return NULL;
};

int main(int argc, const char * argv[]) {
    thread_pool_t *pool = thread_pool_create(500, 1000);
    for(long i = 0; i < 2000; i++){
        thread_pool_add(pool, (void *)print, (void *)i);
        sleep(1);
    }
    printf("Finish Adding Job:)\n");
    
    thread_pool_destroy(pool);
    thread_pool_add(pool, (void *)print, (void *)2);
    printf("I'll exit:)\n");
    exit(0);
}
