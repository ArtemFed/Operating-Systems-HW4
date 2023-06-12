#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/time.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define NUM_FLOWERS 20
#define GARDENER_SLEEP 1

void sigfunc(int sig) {
    if (sig != SIGINT && sig != SIGTERM) {
        return;
    }

    printf("Sig finished\n");
    exit(10);
}

struct ConnectionArgs {
    int client_type;
    int gardener_id;
    int index;
};

int main(int argc, char const *argv[]) {
    unsigned short server_port;
    const char *server_ip;

    int sock = 0;
    struct sockaddr_in serv_addr;

    struct timeval tv;
    gettimeofday(&tv, NULL);
    int milliseconds = tv.tv_usec / 1000;
    srand(milliseconds);

    // Получаем значения port и server IP с помощью аргументов командной строки
    if (argc != 3) {
        printf("Args: <port> <SERVER_IP>\n");
        return -1;
    }

    server_port = atoi(argv[1]);
    server_ip = argv[2];

    // Создаем UDP сокет
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }

    memset(&serv_addr, 0, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(server_port);

    // Устанавливаем адрес сервера
    if (inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    int gardener_id = rand() % 90 + 10;
    socklen_t addr_len = sizeof(serv_addr);

    bool server_is_connected = true;
    printf("Gardener №%d started\n", gardener_id);

    while (server_is_connected) {
        int server_answer_1 = 0;

        for (int i = 0; i < NUM_FLOWERS / 4; ++i) {
            if (i % 3 == 0) {
                sleep(GARDENER_SLEEP);
            }
            // Отправляем значения index на сервер
            int index = rand() % NUM_FLOWERS;
            printf("Gardener №%d: Choose flower №%d\n", gardener_id, index);

            struct ConnectionArgs connection_args;
            connection_args.client_type = 'g';
            connection_args.gardener_id = gardener_id;
            connection_args.index = index;

            sendto(sock, &connection_args, sizeof(connection_args), 0, (struct sockaddr *) &serv_addr, sizeof(serv_addr));

            int rec_len = recvfrom(sock, &server_answer_1, sizeof(server_answer_1), 0, (struct sockaddr *) &serv_addr, &addr_len);
            if (rec_len < 0 ) {
                printf("Error while reading");
                server_is_connected = false;
                break;
            }

            if (server_answer_1 == 0) {
                printf("Gardener №%d: WATERED flower №%d\n", gardener_id, index);
            } else if (server_answer_1 == 1) {
                printf("Gardener №%d: already DEAD flower №%d\n", gardener_id, index);
            } else if (server_answer_1 == 2) {
                printf("Gardener №%d: already WATERED flower №%d\n", gardener_id, index);
            } else {
                server_is_connected = false;
                break;
            }
        }

        sleep(GARDENER_SLEEP  * 2);
    }

    return 0;
}
