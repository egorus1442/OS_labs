#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>

#define BUFFER_SIZE 256

int client_fd;
char login[BUFFER_SIZE];

void handle_signal(int sig) {
    printf("Client shutting down...\n");
    close(client_fd);
    char client_fifo[BUFFER_SIZE];
    sprintf(client_fifo, "/tmp/chat_client_%s", login);
    unlink(client_fifo);
    exit(0);
}

// Функция для чтения входящих сообщений в отдельном потоке
void* receive_messages(void* arg) {
    char buffer[BUFFER_SIZE];
    while (1) {
        int bytes_read = read(client_fd, buffer, BUFFER_SIZE);
        if (bytes_read > 0) {
            printf("\n%s\n", buffer); // Выводим сообщение
            printf("Enter command (create_group, join_group, send, send_group, exit): ");
            fflush(stdout); // Очищаем буфер вывода
        }
        usleep(100000); // Небольшая задержка для уменьшения нагрузки на CPU
    }
    return NULL;
}

int main() {
    signal(SIGINT, handle_signal);

    printf("Enter your login: ");
    scanf("%s", login);

    // Подключаемся к серверу
    int server_fd = open("/tmp/chat_server", O_WRONLY);
    char buffer[BUFFER_SIZE];
    sprintf(buffer, "CONNECT %s", login);
    write(server_fd, buffer, strlen(buffer) + 1);

    // Создаем именованный канал для получения сообщений
    char client_fifo[BUFFER_SIZE];
    sprintf(client_fifo, "/tmp/chat_client_%s", login);
    mkfifo(client_fifo, 0666);

    // Открываем канал для чтения
    client_fd = open(client_fifo, O_RDONLY | O_NONBLOCK);

    printf("Connected to server. You can start sending messages.\n");

    // Создаем поток для чтения входящих сообщений
    pthread_t thread;
    pthread_create(&thread, NULL, receive_messages, NULL);

    while (1) {
        // Ввод команды
        printf("Enter command (create_group, join_group, send, send_group, exit): ");
        char command[BUFFER_SIZE];
        scanf("%s", command);

        if (strcmp(command, "create_group") == 0) {
            // Создание группы
            char group_name[BUFFER_SIZE];
            printf("Enter group name: ");
            scanf("%s", group_name);
            sprintf(buffer, "CREATE_GROUP %s %s", login, group_name);
            write(server_fd, buffer, strlen(buffer) + 1);
        } else if (strcmp(command, "join_group") == 0) {
            // Присоединение к группе
            char group_name[BUFFER_SIZE];
            printf("Enter group name: ");
            scanf("%s", group_name);
            sprintf(buffer, "JOIN_GROUP %s %s", login, group_name);
            write(server_fd, buffer, strlen(buffer) + 1);
        } else if (strcmp(command, "send") == 0) {
            // Отправка личного сообщения
            char recipient[BUFFER_SIZE];
            char text[BUFFER_SIZE];
            printf("Enter recipient and message: ");
            scanf("%s %[^\n]", recipient, text);
            sprintf(buffer, "SEND %s %s [%s]: %s", login, recipient, login, text);
            write(server_fd, buffer, strlen(buffer) + 1);
        } else if (strcmp(command, "send_group") == 0) {
            // Отправка сообщения в группу
            char group_name[BUFFER_SIZE];
            char text[BUFFER_SIZE];
            printf("Enter group name and message: ");
            scanf("%s %[^\n]", group_name, text);
            sprintf(buffer, "SEND_GROUP %s %s [%s]: %s", login, group_name, login, text);
            write(server_fd, buffer, strlen(buffer) + 1);
        } else if (strcmp(command, "exit") == 0) {
            // Отправка команды отключения
            sprintf(buffer, "DISCONNECT %s", login);
            write(server_fd, buffer, strlen(buffer) + 1);
            break; // Завершаем цикл и завершаем программу
        }
    }

    close(client_fd);
    unlink(client_fifo);
    close(server_fd);
    return 0;
}