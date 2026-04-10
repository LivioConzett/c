#include <stdio.h>

#include "pqd_server.h"

int main(){


    clientstate_t clientStates[MAX_CLIENTS];

    poll_loop(PORT, clientStates);

    return 0;
}