/*
* A server using poll.
* Inspired by Lowlevel (https://www.youtube.com/@LowLevelTV)
*
*
*/

#ifndef PQD_SERVER_H
#define PQD_SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>



#define BACKLOG 10


// -----------------------------------------------------------------------------------
// structure declarations
// -----------------------------------------------------------------------------------

typedef enum {
    STATE_NEW,
    STATE_CONNECTED,
    STATE_DISCONNECTED,
    STATE_HELLO,
    STATE_MSG,
    STATE_GOODBYE
} sv_state_e;


typedef struct {
    int fd;
    sv_state_e state;
    char *buffer;
} sv_clientstate_t;


typedef struct {
    short port;
    int max_clients;
    int buffer_size;
} sv_settings_t;


// -----------------------------------------------------------------------------------
// function declarations
// -----------------------------------------------------------------------------------




// -----------------------------------------------------------------------------------
// function definition
// -----------------------------------------------------------------------------------

// #ifdef PQD_SERVER_IMPLEMENTATION

/**
 * See function declaration
 */
void sv_init_clients(sv_clientstate_t *states, sv_settings_t settings){

    for(int i = 0; i < settings.max_clients; i++){
        states[i].fd = -1;
        states[i].state = STATE_NEW;
        states[i].buffer = malloc(settings.buffer_size);
        memset(states[i].buffer, '\0', settings.buffer_size);
    }
}

/**
 * See function declaration
 */
int sv_find_free_slot(sv_clientstate_t* states, int max_clients){
    for(int i = 0; i < max_clients; i++){
        if(states[i].fd == -1){
            return i;
        }
    }
    return -1;
}

/**
 * See function declaration
 */
int sv_find_slot_by_fd(sv_clientstate_t* states, int fd, int max_clients){
    for(int i = 0; i < max_clients; i++){
        if(states[i].fd == fd){
            return i;
        }
    }
    return -1;
}

/**
 * See function declaration
 */
void sv_server(sv_settings_t settings){
	int listen_fd, conn_fd, freeSlot;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    struct pollfd fds[settings.max_clients + 1];
    int nfds = 1;
    int opt = 1;

    sv_clientstate_t clientStates[settings.max_clients];

    sv_init_clients(clientStates, settings);


    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(listen_fd == -1) {
        perror("socket");
        return;
    }

    if(setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))){
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(settings.port);

    if (bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    if (listen(listen_fd, BACKLOG) == -1) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", settings.port);

    memset(fds, 0, sizeof(fds));
    fds[0].fd = listen_fd;
    fds[0].events = POLLIN;
    nfds = 1;


    while(1){

        int ii = 1;
        for(int i = 0; i < settings.max_clients; i++){
            if(clientStates[i].fd != -1){
                fds[ii].fd = clientStates[i].fd;
                fds[ii].events = POLLIN;
                ii++;
            }
        }

        // wait for an event on one of the sockets
        int n_events = poll(fds, nfds, -1);
        if(n_events == -1){
            perror("poll");
            exit(EXIT_FAILURE);
        }

        // Check for new connections
        if(fds[0].revents & POLLIN){
            if((conn_fd = accept(listen_fd, (struct sockaddr*)&client_addr, &client_len)) == -1){
                perror("accept");
                continue;
            }

            printf("New connection from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

            freeSlot = sv_find_free_slot(clientStates, settings.max_clients);
            if(freeSlot == -1){
                printf("server full: closing new connection!\n");
                close(conn_fd);
            } else {
                clientStates[freeSlot].fd = conn_fd;
                clientStates[freeSlot].state = STATE_HELLO;
                nfds++;
                printf("Slot %d has fd %d\n", freeSlot, clientStates[freeSlot].fd);
            }

            n_events--;

        }

        for(int i = 1; i <= nfds && n_events > 0; i++){
            if(fds[i].revents & POLLIN){
                n_events--;

                int fd = fds[i].fd;
                int slot = sv_find_slot_by_fd(clientStates, fd, settings.max_clients);
                ssize_t bytes_read = read(fd, clientStates[slot].buffer, settings.buffer_size);
                if(bytes_read <= 0){
                    close(fd);
                    if(slot == -1){
                        printf("Tried to close fd that doesn't exist!\n");
                    } else {
                        clientStates[slot].fd = -1;
                        clientStates[slot].state = STATE_DISCONNECTED;
                        printf("Client disconnected or error\n");
                        nfds--;
                    }
                } else {
                    // TODO: add handle client stuff
                    printf("%s", clientStates[slot].buffer);
                }
            }
        }
    }

    return;
}



// #endif
#endif