#include <stdio.h>

#include "pqd_server.h"


void test_function(sv_clientstate_t *arg){

    printf("%s\n", arg->buffer);

}



int main(){


    sv_settings_t settings = {
        .port = 9090,
        .max_clients = 256,
        .buffer_size = 4090,
        .backlog = 10,
        .function = test_function
    };

    sv_server(settings);

    return 0;
}