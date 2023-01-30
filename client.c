#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <netdb.h>


int sockfd = 0;
char *name;

struct hostent *server;
pthread_mutex_t mutex;


int readFromSocket(uint32_t socketFD, char *buffer, uint32_t bytesToRead) {

    char *ptr = (char *) buffer;
    while (bytesToRead > 0) {
        int i = recv(socketFD, ptr, bytesToRead, 0);
        if (i<1) return EXIT_FAILURE;
        ptr += i;
        bytesToRead -= i;
    }
    return EXIT_SUCCESS;
}


int sendFromSocket(int socketFD, char *buffer, uint32_t bytesToRead) {
    char *ptr = (char *) buffer;
    while (bytesToRead > 0) {
        int i = send(socketFD, ptr, bytesToRead, 0);
        if (i < 1) return EXIT_FAILURE;
        ptr += i;
        bytesToRead -= i;
    }
    return EXIT_SUCCESS;

}

void send_message(char *mes) {
    mes[strcspn(mes, "\n")] = 0;
    uint32_t size_mes = strlen(mes);
    size_mes = htonl(size_mes);
    int k = sendFromSocket(sockfd, (void *) &size_mes, 4);
    if (k == 0) {
        int q = sendFromSocket(sockfd, mes, strlen(mes));
        // printf("Отправил: %s\n",mes);
        if (q != 0) {
            perror("2. ERROR writing to socket");
        }
    }
}

void *send_msg_handler() {
    size_t lenght_m = 1;
    char *message;
    char *Button_m;
    Button_m = (char *) calloc((lenght_m), sizeof(char));
    while (1) {
        while (getline(&Button_m, &lenght_m, stdin) && !feof(stdin)) {
            if (strcmp(Button_m, "m\n") == 0) {
                pthread_mutex_lock(&mutex);
                size_t mes_size = 4;
                size_t mes;
                message = (char *) calloc((mes_size), sizeof(char));
                mes = getline(&message, &mes_size, stdin);
                if (mes > 0) {
                    send_message(name);
                    send_message(message);

                }
                pthread_mutex_unlock(&mutex);
            }
        }
        if (feof(stdin)) {
            memset(name, 0, strlen(name));
            memset(message, 0, strlen(message));
            exit(1);
        }
    }
}

void *recv_msg_handler() {
    char *nick, *msg, *time_s;
    uint32_t size_nick;

    while (1) {
         int receive = readFromSocket(sockfd, (void *) &size_nick, 4);
        if (receive == EXIT_SUCCESS) {
            size_nick = ntohl(size_nick);
            if (size_nick > 0) {
                nick = (char *) calloc((size_nick + 1), sizeof(char));
                int n = readFromSocket(sockfd, nick, size_nick);
                if (n != EXIT_SUCCESS) {
                    continue;
                }
                uint32_t size_ms;
                int r = readFromSocket(sockfd, (void *) &size_ms, 4);
                if (r != EXIT_SUCCESS) {
                    continue;
                }
                size_ms = ntohl(size_ms);
                if (size_ms > 0) {
                    msg = (char *) calloc((size_ms + 1), sizeof(char));
                    int m = readFromSocket(sockfd, msg, size_ms);
                    if (m != EXIT_SUCCESS) {
                        continue;
                    }
                    uint32_t size_time;
                    int t = readFromSocket(sockfd, (void *) &size_time, 4);
                    if (t != EXIT_SUCCESS) {
                        continue;
                    }
                    size_time = ntohl(size_time);
                    time_s = (char *) calloc((size_time + 1), sizeof(char));
                    int k = readFromSocket(sockfd, time_s, size_time);
                    if (k != EXIT_SUCCESS) {
                        continue;
                    }
                    pthread_mutex_lock(&mutex);
                    printf("{%s} [%s] %s\n", time_s, nick, msg);
                    pthread_mutex_unlock(&mutex);
                    memset(nick, 0, strlen(nick));
                    memset(time_s, 0, strlen(time_s));
                    memset(msg, 0, strlen(msg));
                }
            }
        }
        else {
            close(sockfd);
            perror("Socket closed");
            exit(0);
        }
    }

}


int main(int argc, char **argv) {

    struct sockaddr_in serv_addr;
    if (argc < 4) {
        printf("Usage: %s <localhost> <port> <name>\n", argv[0]);
        return EXIT_FAILURE;
    }
    server = gethostbyname(argv[1]);
   uint16_t port = atoi(argv[2]);
    name = argv[3];

    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0) {
        perror("ERROR opening socket");
        exit(1);
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    memcpy(server->h_addr_list[0], (char *) &serv_addr.sin_addr.s_addr, (size_t) server->h_length);
    serv_addr.sin_port = htons(port);

    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        perror("ERROR connecting");
        exit(1);
    }

    //printf("3\n");
    pthread_t send_msg_thread;
    pthread_create(&send_msg_thread, NULL, &send_msg_handler, NULL);
    // printf("4\n");
    pthread_t recv_msg_thread;
    pthread_create(&recv_msg_thread, NULL, &recv_msg_handler, NULL);
    pthread_join(send_msg_thread, NULL);
    pthread_join(recv_msg_thread, NULL);

    // pthread_mutex_destroy(&mutex);
    //pthread_detach(pthread_self());
    close(sockfd);

    return EXIT_SUCCESS;
}
