#include <stdio.h>

#include "pqd_server.h"

int main(){


    sv_clientstate_t *clientStates = NULL;

    sv_server(PORT, &clientStates);

    return 0;
}