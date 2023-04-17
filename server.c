#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <inttypes.h>
#include <netinet/in.h>
#include <pthread.h>
#include <arpa/inet.h>

#define PORT 9000



void setupstruct(struct sockaddr_in *s);
void* client_handler(void* socket);

struct arg_struct{
    int socket;
    char usern[64]; 
};

int fds[128];

int main(){
    memset(fds, -1, sizeof(fds));
    int client_socket; 
    
    
    int s_socket = socket(AF_INET, SOCK_STREAM, 0);
    printf("Our socket: %d\n", s_socket);


    struct sockaddr_in s_address;
    setupstruct(&s_address); 
     
    
    if((bind(s_socket, (struct sockaddr*) &s_address, sizeof(s_address))) < 0){
        printf("bind failed error code: %d\n", errno);
        perror("Error printed by perror");
        exit(errno);
    }

    listen(s_socket, 10);

    while(true){
        struct arg_struct *params = malloc(sizeof(struct arg_struct));
        client_socket = accept(s_socket, NULL, NULL);

        char username[64];
        recv(client_socket, username, sizeof(username), 0);

        printf("Client connected! from: %s:%d, their username: %s\n", inet_ntoa(s_address.sin_addr), ntohs(s_address.sin_port), username);
        
        params->socket = client_socket;
        strncat(params->usern, username, sizeof(username));

        fds[client_socket-4] = client_socket; // basic way of tracking the fds, client fds will always be > 3

        bzero(username, sizeof(username)); // this call probably isn't needed

        pthread_t newThread; 
        pthread_create(&newThread, NULL, client_handler, (void*) params); // so we can handle multiple clients simultaneously

     
        printf("fds: ");

        for(int i = 0; i<sizeof(fds); i++){
            if(fds[i] == -1){
                break;
            }
            else{
                printf("%d, ", fds[i]);
            }
        }
        printf("\n");
    }
    printf("Shutting off, bye!"); // there's actually no way to reach this, have to add some server commands 
    close(s_socket);
    return 0;
}

void setupstruct(struct sockaddr_in *s){
    s->sin_family = AF_INET;
    s->sin_port = htons(PORT);
    s->sin_addr.s_addr = INADDR_ANY;
}

void* client_handler(void* args){

    struct arg_struct *the_args = (struct arg_struct *) args;

    char buffer[512]; // the message received from a client
    char sendback[1024]; // what's actually sent to the other clients

    int socket = the_args->socket; 

    int usernamesize = strlen(the_args->usern);

    printf("Client handler: %s\n", the_args->usern);

    strncpy(sendback, the_args->usern, usernamesize);
    
    printf("fd: %d\n", socket); 
    while(true){
        int bytes = recv(socket, buffer, sizeof(buffer), 0);
        strncat(sendback, buffer, sizeof(sendback));
        if(bytes > 0){
            printf("%s\n", sendback);
            for(int i = 0; i<128; i++){
                if(fds[i] == -1){
                    break; 
                }
                if(fds[i] == -2){
                    continue;
                }
                if(fds[i] == socket){ // don't echo it back to the sender of course
                    continue; 
                }
                else{
                    if(send(fds[i], sendback, sizeof(sendback), 0) < 0){
                        printf("send error happened on fd: %d, errno: %d\n", fds[i], errno);
                    }
                }
            }
            bzero(buffer, sizeof(buffer));
            memset(sendback+usernamesize, 0, sizeof(sendback)-usernamesize);
        }
        else if (bytes == -1){
            fds[socket-3] = -2; // set to -2 so it can be treated as an available fd and not the end of the list
            printf("errno: %d\n", errno);
            perror("Error printed by perror");
            break;
    
        }
    }
    close(socket);
    exit(0);
}




