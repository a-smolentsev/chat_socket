#define _POSIX_C_SOURCE 200112L
#undef _GNU_SOURCE

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#define MAX_CLIENTS 100
static _Atomic unsigned int cli_count = 0;
static int uid = 10;


typedef struct{
    //struct sockaddr_in address;
    int sockfd;
    int uid;
    char name[32];
} client_t;

client_t *clients[MAX_CLIENTS];

pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

void print_client_addr(struct sockaddr_in addr){
    printf("%d.%d.%d.%d",
           addr.sin_addr.s_addr & 0xff,
           (addr.sin_addr.s_addr & 0xff00) >> 8,
           (addr.sin_addr.s_addr & 0xff0000) >> 16,
           (addr.sin_addr.s_addr & 0xff000000) >> 24);
}


void queue_add(client_t *cl){
    pthread_mutex_lock(&clients_mutex);

    for(int i=0; i < MAX_CLIENTS; ++i){
        // printf("connect %d\n",i);
        if(!clients[i]){
            clients[i] = cl;
            break;
        }
    }

    pthread_mutex_unlock(&clients_mutex);
}


void queue_remove(int uid){
    pthread_mutex_lock(&clients_mutex);

    for(int i=0; i < MAX_CLIENTS; ++i){
        if(clients[i]){
            if(clients[i]->uid == uid){
                clients[i] = NULL;
                 printf("delete %d\n",i);
                break;
            }
        }
    }

    pthread_mutex_unlock(&clients_mutex);
}



int readFromSocket(int socketFD, char * buffer, uint32_t bytesToRead){


    char *ptr = (char*) buffer;
    while (bytesToRead > 0)
    {
        int i = recv(socketFD, ptr, bytesToRead,0);
        if (i < 1)  { return EXIT_FAILURE;}
        ptr += i;
        bytesToRead -= i;
    }
    return EXIT_SUCCESS;

}

int sendFromSocket(int socketFD, char * buffer, uint32_t bytesToRead){
    char *ptr = (char*) buffer;
    while (bytesToRead > 0)
    {
        int i = send(socketFD, ptr, bytesToRead,0);
        if (i < 1){ printf("send: %d",i);return EXIT_FAILURE;}
        ptr += i;
        bytesToRead -= i;
    }
    return EXIT_SUCCESS;



}

void send_mess(char *mes,int sockfd)
{  uint32_t size_mes=strlen(mes);
    size_mes=htonl(size_mes);
    int k=sendFromSocket(sockfd,(void*)&size_mes,4);
    if (k==0)
    {
        int q=sendFromSocket(sockfd,mes,strlen(mes));// исправил +1
        if (q !=0) {
            perror("Server ERROR writing to socket");

        }
    }
}

void send_message(char *s){
    pthread_mutex_lock(&clients_mutex);
    for(int i=0; i<MAX_CLIENTS; ++i){
        if(clients[i]){
            send_mess(s,clients[i]->sockfd);
           /// printf("Отправил клиентам: %s\n", s);

        }
    }
    pthread_mutex_unlock(&clients_mutex);
}


void *handle_client(void *arg) {
    cli_count++;
    client_t *cli = (client_t *) arg;
    int leave_flag = 0;

    while (1) {
        if (leave_flag) {
            break;
        }
        uint32_t size_bf;
        char *nick;

        int receive = readFromSocket(cli->sockfd, (void *) &size_bf, 4);
       ///printf("receive: %d\n", receive);
        if (receive == EXIT_SUCCESS) {
            size_bf = ntohl(size_bf);
            if (size_bf > 0) {
                nick = (char *) calloc((size_bf + 1), sizeof(char));
                int n = readFromSocket(cli->sockfd, nick, size_bf);
                if (n != EXIT_SUCCESS) {
                    continue;
                }
                uint32_t size_ms;
                char *ms;
                int r = readFromSocket(cli->sockfd, (void *) &size_ms, 4);
                if (r != EXIT_SUCCESS) {
                    continue;
                }
                size_ms = ntohl(size_ms);
                if (size_ms > 0) {
                    ms = (char *) calloc((size_ms + 1), sizeof(char));
                    readFromSocket(cli->sockfd, ms, size_ms);
                    //// printf("сообщение: %s\n", ms);
                    char *times;
                    times = (char *) malloc(6 * sizeof(char));
                    time_t rawtime;
                    struct tm *timeinfo;
                    time(&rawtime);
                    timeinfo = localtime(&rawtime);
                    sprintf(times, "%02d:%02d", timeinfo->tm_hour, timeinfo->tm_min);
                    send_message(nick);
                    send_message(ms);
                    send_message(times);
                    memset(nick, 0, strlen(nick));
                    memset(ms, 0, strlen(ms));
                    memset(times, 0, strlen(times));
                }
            }
        }
        else {
            leave_flag = 1;
        }
    }

    close(cli->sockfd);
    queue_remove(cli->uid);
    free(cli);
    cli_count--;


    return NULL;
}

int main(int argc, char **argv){
    struct addrinfo hint = {
            .ai_family = AF_INET,
            .ai_socktype = SOCK_STREAM,
            .ai_flags = AI_PASSIVE
    };
    if(argc != 2){
        printf("Usage: %s <port>\n", argv[0]);
        return EXIT_FAILURE;
    }


    pthread_t tid;
    const char* const PORT = argv[1];

    struct addrinfo* result = NULL;
    int rc = 0;
    if ((rc = getaddrinfo(NULL, PORT, &hint, &result)) != 0) {
        fprintf(stderr, "getaddrinfo %s\n", gai_strerror(rc));
        return 1;
    }



    int sockfd = -1;
    for (struct addrinfo* p = result; p; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("socket");
            continue;
        }

        if ((rc = bind(sockfd, p->ai_addr, p->ai_addrlen) != 0)) {
            perror("bind");
            close(sockfd);
            sockfd = -1;
        }
    }


    if (sockfd == -1) {
        fprintf(stderr, "fail");
        freeaddrinfo(result);
        return 1;
    }

    freeaddrinfo(result);


    if ((rc = listen(sockfd, 5)) != 0) {
        perror("listen");
        close(sockfd);
        return 1;
    }


    while(1){
        struct sockaddr_storage storage;
        unsigned int size = 0;
        int cfd = accept(sockfd, (struct sockaddr *)&storage, &size);
        if (cfd == -1) {
            perror("accept");
            continue;
        }



        if((cli_count + 1) == MAX_CLIENTS){
            printf("Max clients reached. Rejected: ");

            close(cfd);
            continue;
        }

        client_t *cli = (client_t *) malloc(sizeof(client_t));

        cli->sockfd = cfd;
        cli->uid = uid++;

        queue_add(cli);
        pthread_create(&tid, NULL, &handle_client, (void *) cli);
        pthread_detach(tid);

    }

    return EXIT_SUCCESS;
}
