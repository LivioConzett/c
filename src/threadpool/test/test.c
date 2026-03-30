#include <stdio.h>
#include <assert.h>

#define PQD_THREADPOOL_IMPLEMENTATION
#include "pqd_threadpool.h"


void print_error(char *msg){
    printf("\e[0;31m" "%s" "\e[0m" "\n", msg);
}

void print_success(char *msg){
    printf("\e[0;32m" "%s" "\e[0m" "\n", msg);
}

void test_int(int is, int expected, char *msg){

    if(is == expected){
        print_success(msg);
    } else {
        print_error(msg);
    }
}


void test_tp_init(){

    printf("Testing tp_init\n");

    tp_pool_t pool = {0};
    pool.stop = 1;

    tp_init(10, 10, &pool);

    test_int(pool.stop, 0, "pool.stop == 0");


}





int main(){

    printf("Starting test for threadpool\n");

    test_tp_init();

    return 0;
}