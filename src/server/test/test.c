#include <stdio.h>

#include "pqd_server.h"

int main(){


    sv_settings_t settings = {
        .port = 9090,
        .max_clients = 256,
        .buffer_size = 4090
    };

    sv_server(settings);

    return 0;
}