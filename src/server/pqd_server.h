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
    int backlog;
    void (*function)(sv_clientstate_t *client);
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

    struct pollfd poll_files[settings.max_clients + 1];
    int num_of_poll_files = 1;
    int opt = 1;

    // create the clientState list
    sv_clientstate_t clientStates[settings.max_clients];
    sv_init_clients(clientStates, settings);

    // set the socket
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(listen_fd == -1) {
        perror("socket");
        return;
    }

    // set the socket options
    if(setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))){
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    // set the settings for the server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(settings.port);

    // bind the listen socket 
    if (bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    // listen on the socket
    if (listen(listen_fd, settings.backlog) == -1) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", settings.port);

    // place the listen fd into the first spot of the array
    memset(poll_files, 0, sizeof(poll_files));
    poll_files[0].fd = listen_fd;
    poll_files[0].events = POLLIN;
    num_of_poll_files = 1;


    // listen for connections in an endless loop
    while(1){

        // add the clients that are connected to
        // the array of file descriptors used to poll
        int ii = 1;
        for(int i = 0; i < settings.max_clients; i++){
            if(clientStates[i].fd != -1){
                poll_files[ii].fd = clientStates[i].fd;
                poll_files[ii].events = POLLIN;
                ii++;
            }
        }

        // poll the files, to see if one of them has and event
        int n_events = poll(poll_files, num_of_poll_files, -1);
        if(n_events == -1){
            perror("poll");
            exit(EXIT_FAILURE);
        }

        // Check for new connections.
        // The first file in the list is the listening socket.
        if(poll_files[0].revents & POLLIN){
            if((conn_fd = accept(listen_fd, (struct sockaddr*)&client_addr, &client_len)) == -1){
                perror("accept");
                continue;
            }

            printf("New connection from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

            // check for a free clientSte to place the new connection into
            freeSlot = sv_find_free_slot(clientStates, settings.max_clients);
            if(freeSlot == -1){
                printf("server full: closing new connection!\n");
                close(conn_fd);
            } else {
                clientStates[freeSlot].fd = conn_fd;
                clientStates[freeSlot].state = STATE_HELLO;
                num_of_poll_files++;
                printf("Slot %d has fd %d\n", freeSlot, clientStates[freeSlot].fd);
            }

            n_events--;

        }

        // go through the rest of the files to see if any of the connections have an event
        for(int i = 1; i <= num_of_poll_files && n_events > 0; i++){
            if(poll_files[i].revents & POLLIN){
                n_events--;

                int fd = poll_files[i].fd;
                int slot = sv_find_slot_by_fd(clientStates, fd, settings.max_clients);
                ssize_t bytes_read = read(fd, clientStates[slot].buffer, settings.buffer_size);
                if(bytes_read <= 0){
                    close(fd);
                    if(slot == -1){
                        printf("Tried to close fd that doesn't exist!\n");
                    } else {
                        // reset the clientState to be used again
                        clientStates[slot].fd = -1;
                        clientStates[slot].state = STATE_DISCONNECTED;
                        printf("Client disconnected or error\n");
                        num_of_poll_files--;
                    }
                } else {
                    // handle the client data
                    (*(settings.function))(&clientStates[slot]);
                }
            }
        }
    }

    return;
}



// #endif
#endif