#include <stdio.h>
#include <assert.h>
#include <pthread.h>

#define PQD_THREADPOOL_IMPLEMENTATION
#include "pqd_threadpool.h"


void print_error(char *msg){
    printf("\e[0;31m" "%s" "\e[0m" "\n", msg);
}

void print_success(char *msg){
    printf("\e[0;32m" "%s" "\e[0m" "\n", msg);
}

void test_int_is(int is, int expected, char *msg){

    if(is == expected){
        print_success(msg);
    } else {
        print_error(msg);
    }
}

void test_int_isnot(int is, int not, char *msg){

    if(is != not){
        print_success(msg);
    } else {
        print_error(msg);
    }
}

/**
 * Test the init of the thread pool
 */
void test_tp_init(){

    printf("Testing tp_init\n");

    tp_pool_t pool = {0};
    pool.stop = 1;
    pool.queue_back = 1;
    pool.queue_front = 1;
    pool.queued = 1;

    tp_init(10, 20, &pool);

    test_int_is(pool.stop, 0, "pool.stop == 0");
    test_int_is(pool.queue_back, 0, "pool.back == 0");
    test_int_is(pool.queue_front, 0, "pool.front == 0");
    test_int_is(pool.queued, 0, "pool.queued == 0");
    test_int_is(pool.task_amount, 20, "pool.task_amount == 20");
    test_int_is(pool.thread_amount, 10, "pool.thread_amount == 10");
    test_int_isnot(pool.threads, NULL, "pool.threads != NULL");
    test_int_isnot(pool.task_queue, NULL, "pool.task_queue != NULL");

    tp_destroy(&pool);
}


int test_number[10];

void test_function(void* arg){
    int index = *(int*)arg;
    test_number[index] = index;
    free(arg);
}

/**
 * Test adding fo tasks
 */
void test_task_adding(){

    printf("Testing adding tasks\n");

    tp_pool_t pool;
    tp_init(3, 10, &pool);

    for(int i = 0; i < 10; i++){

        int *num = (int *)malloc(sizeof(int));
        *num = i;

        tp_add_task(&pool, test_function, (void *)num);
    }

    sleep(5);

    for(int i = 0; i < 10; i++){
        test_int_is(test_number[i], i, "number test");
    }

    tp_destroy(&pool);
}


/**
 * Main function
 */
int main(){

    printf("Starting test for threadpool\n");

    test_tp_init();
    test_task_adding();

    return 0;
}