#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>

#include <signal.h>
#include <sys/types.h>
#include <stdbool.h>

#define NUM_FLOWERS 20
#define HEIGHT 4 // Длина клумбы
#define WIDTH 5 // Ширина клумбы

#define FLOWERS_SLEEP 10
#define GARDENER_SLEEP 1


// Состояние цветка
typedef enum {
    WATERED,// Политый
    FADED,  // Увядающий
    DEAD    // Мертвый
} FlowerState;

// Структура, хранящая информацию о состоянии клумбы и днях
typedef struct {
    FlowerState flowers[NUM_FLOWERS];
    int all_days_count;
    int cur_day;
    bool is_started;
} Garden;

struct ConnectionArgs {
    int client_type;
    int gardener_id;
    int index;
};


Garden *garden;

void sigfunc(int sig) {
    if (sig != SIGINT && sig != SIGTERM) {
        return;
    }

    printf("Sig finished\n");
    exit(10);
}


char *printGarden(Garden *garden_for_print) {
    int count_of_watered = 0;
    int count_of_faded = 0;
    int count_of_dead = 0;
    char *answer = (char *) malloc(HEIGHT * WIDTH * 2 + 200); // выделяем память под строку ответа
    char *temp_answer = (char *) malloc(10); // временная строка для форматирования чисел
    for (int i = 0; i < HEIGHT; i++) {
        for (int j = 0; j < WIDTH; j++) {
            if (garden_for_print->flowers[i * HEIGHT + j] == WATERED) {
                count_of_watered++;
                strcat(answer, "W ");
            } else if (garden_for_print->flowers[i * HEIGHT + j] == FADED) {
                count_of_faded++;
                strcat(answer, "F ");
            } else {
                count_of_dead++;
                strcat(answer, "D ");
            }
        }
        strcat(answer, "\n");
    }
    sprintf(temp_answer, "%d", count_of_watered); // форматирование чисел в строку
    strcat(answer, "Count of Watered: ");
    strcat(answer, temp_answer);
    strcat(answer, "\n");
    sprintf(temp_answer, "%d", count_of_faded);
    strcat(answer, "Count of Faded: ");
    strcat(answer, temp_answer);
    strcat(answer, "\n");
    sprintf(temp_answer, "%d", count_of_dead);
    strcat(answer, "Count of Dead: ");
    strcat(answer, temp_answer);
    strcat(answer, "\n");
    return answer;
}


void *daysProcess() {
    // Код процесса отслеживания состояния цветов на клумбе
    for (int day = 1; day <= garden->all_days_count + 1; ++day) {
        garden->cur_day = day;

        /* Устанавливаем цветам состояния:
         * Политым - явядающие
         * Увядающим - Мёртвые */
        for (int i = 0; i < NUM_FLOWERS; i++) {
            if (garden->flowers[i] == WATERED) {
                garden->flowers[i] = FADED;
            } else if (garden->flowers[i] == FADED) {
                garden->flowers[i] = DEAD;
            }
        }

        if (garden->cur_day > garden->all_days_count) {
            printf("Flowers finished\n");
            break;
        }

        printf(
                "--------------------------------------------------"
                "\nDay: %d / %d started\n",
                day,
                garden->all_days_count
        );

        printf("%s", printGarden(garden));

        // Ждем, пока не истечет время до начала увядания цветов
        sleep(FLOWERS_SLEEP);

        printf("Flowers at night (after Watering):\n");
        printf("%s", printGarden(garden));
    }
    garden->is_started = false;

    printf("\n!ARMAGEDDON ARE STARTING!\n");

    sleep(2);
    printf("Last day:\n");
    printf("%s", printGarden(garden));

    exit(0);
}


int currentCountOfClients = 0;

int main(int argc, char const *argv[]) {
//    signal(SIGINT, sigfunc);
//    signal(SIGTERM, sigfunc);

    unsigned short server_port;
    int all_days_count = 0;
    // Чтение входных данных
    {
        server_port = 8080;
        all_days_count = 3;
        srand(42);
        if (argc >= 2) {
            // Порт сервера
            server_port = atoi(argv[1]);
        }
        if (argc >= 3) {
            // Сколько дней отобразить
            all_days_count = atoi(argv[2]);
        }
        if (argc >= 4) {
            // Рандомизированный ввод
            srand(atoi(argv[3]));
        }
    }

    int server_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(server_addr);
    socklen_t client_len = sizeof(client_addr);
    pthread_t thread_id;

    printf("FLOWER SERVER\n");

    // Создаем TCP сокет
    if ((server_fd = socket(AF_INET, SOCK_DGRAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }


    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(server_port);

    // Привязываем сокет к адресу и порту
    if (bind(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    garden = malloc(sizeof(Garden));

    garden->all_days_count = all_days_count;

    // Инициализируем состояние клумбы
    for (int i = 0; i < NUM_FLOWERS; i++) {
        garden->flowers[i] = WATERED;
    }
    garden->is_started = true;

    // Запускаем процесс Flowers
    printf("Flowers started\n");
    printf("W - WATERED\nF - FADED\nD - DEAD\n");

    printf("Flowers are waiting for Gardeners...\n");

    sleep(2);

    if (pthread_create(&thread_id, NULL, daysProcess, &garden->all_days_count) > 0) {
        perror("could not create thread");
        return 1;
    }

    // Обрабатываем каждое новое подключение в отдельном потоке
    while (garden->is_started) {
        currentCountOfClients++;

        // Принимаем запрос от клиента
        struct ConnectionArgs conn_args;
        recvfrom(server_fd, &conn_args, sizeof(struct ConnectionArgs), 0,
                 (struct sockaddr *) &client_addr, &client_len);

        int client_type = conn_args.client_type;
        int index = conn_args.index;
        int gardener_id = conn_args.gardener_id;

        if (client_type == 'g') {
            int answer = 0;
            // Пробуем полить цветок под index
            if (garden->flowers[index] == DEAD) {
                // Цветок уже умер... RIP
                printf("Gardener №%d: Flower №%d already DEAD\n", gardener_id, index);
                answer = 1;
            } else if (garden->flowers[index] == WATERED) {
                // Цветок уже был полит
                printf("Gardener №%d: Flower №%d already WATERED\n", gardener_id, index);
                answer = 2;
            } else {
                // Поливаем цветок под index
                garden->flowers[index] = WATERED;
                printf("Gardener №%d: Flower №%d was WATERED\n", gardener_id, index);
            }

            if (!garden->is_started) {
                answer = -1;
            }
            sendto(server_fd, &answer, sizeof(answer), 0,
                   (struct sockaddr *) &client_addr, sizeof(client_addr));
        } else {
            char buffer[1024] = {0};

            printf("Send Info to viewer %d\n", gardener_id);
            sprintf((char *) &buffer, "%s", printGarden(garden));
            sendto(server_fd, &buffer, sizeof(buffer), 0,
                   (struct sockaddr *)&client_addr, sizeof(client_addr));
        }
    }


    // Закрываем сокет
    close(server_fd);

    return 0;
}
