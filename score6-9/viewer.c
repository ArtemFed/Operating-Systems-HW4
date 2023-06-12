#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdbool.h>

struct ConnectionArgs {
    int client_type;
    int gardener_id;
    int index;
};

int main(int argc, char const *argv[]) {
    unsigned short serv_port;
    const char *server_ip;

    if (argc == 3) {
        serv_port = atoi(argv[1]);
        server_ip = argv[2];
    } else {
        printf("Args: <port> <SERVER_IP>\n");
        return -1;
    }

    int sockfd = 0;
    struct sockaddr_in serv_addr;
    char buffer[1024] = {0};

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }

    memset(&serv_addr, 0, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(serv_port);

    // Устанавливаем адрес сервера
    if (inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    int type = 'v';

    int viewer_id = rand() % 90 + 10;
    printf("Viewer №%d started\n", viewer_id);

    int messages_count = 0;
    socklen_t server_len = sizeof(serv_addr);

    bool server_is_connected = true;

    while (server_is_connected) {
        struct ConnectionArgs connection_args;
        connection_args.client_type = type;
        connection_args.gardener_id = viewer_id;
        connection_args.index = 0;

        sendto(sockfd, &connection_args, sizeof(connection_args), 0,
               (struct sockaddr *) &serv_addr, sizeof(serv_addr));


        memset(buffer, 0, sizeof(buffer));
        ssize_t recv_len = recvfrom(sockfd, buffer, sizeof(buffer), 0,
                                    (struct sockaddr *)&serv_addr, &server_len);

        if (recv_len == -1) {
            perror("Failed to receive data");
            server_is_connected = false;
            continue;
        }

        printf("----------------------------------\nMessage №%d\n", ++messages_count);
        printf("Server response: \n%s\n", buffer);

        sleep(4);
    }

    close(sockfd);

    return 0;
}
