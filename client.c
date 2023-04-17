#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>

#include <sys/socket.h>
#include <inttypes.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>

#define PORT 9000

void setupstruct(struct sockaddr_in *s);
void* client_receiver(void* socket); 


int main(){
    int connection;
    char buffer[512]; 
    char username[64];
    int n_socket = socket(AF_INET, SOCK_STREAM, 0);

    printf("Hello! Type your username here, maximum 64 characters:\n");
    if(fgets(username, sizeof(username), stdin) == NULL){
        printf("username error, bye!");
        exit(-1);
    }
    printf("Your username: %s", username);

    username[strcspn(username, "\n")] = ':';
    username[strlen(username)] = ' '; 

    struct sockaddr_in s_address;
    setupstruct(&s_address);
    
    if((connection = connect(n_socket, (struct sockaddr* ) &s_address, sizeof(s_address))) < 0){
        printf("Connection failed :(");
        exit(connection);
    }
    else{
        printf("Connected!\n");
    }
    
    send(n_socket, &username, sizeof(username), 0);

    printf("%s", buffer);
    bzero(buffer, 512);

    pthread_t receiverThread;
    receiverThread = pthread_create(&receiverThread, NULL, client_receiver, (void*) &n_socket);
    
    printf("Start typing!\n");
    while(true){
        fgets(buffer, 512, stdin);
        if(strstr(buffer, "//disconnect//") != NULL){ // basic way of disconnecting, have to find something better
            printf("Disconnect flag received, bye!\n");
            break; 
        }
        else{
            send(n_socket, buffer, sizeof(buffer), 0);
            bzero(buffer, 512);
        }
    }
    printf("Disconnecting, bye!\n");
    close(n_socket);
}

void setupstruct(struct sockaddr_in *s){ 
    s->sin_family = AF_INET;
    s->sin_port = htons(PORT);
    s->sin_addr.s_addr = INADDR_ANY;
}

void* client_receiver(void* s){ // gets anything from server to print onto the command line
    char buffer[1024]; 
    int* intsocket = (int*) s; 
    while(true){
        int bytes = recv(*intsocket, buffer, sizeof(buffer), 0);
        if(bytes > 0){
            printf("%s\n", buffer);
        }
        else if (bytes == -1){
            if(errno == ECONNRESET){ // probably have to find a better way to handle server shutting down
                printf("Server is offline :(");
                break;
            }
            else{
                printf("Something has gone very wrong, error no: %d\n", errno);
                perror("Error"); 
                break;
            }
        }
        bzero(buffer, sizeof(buffer));
    }
    exit(0);
}

