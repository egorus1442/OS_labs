#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>

#define MAX_CLIENTS 10
#define MAX_GROUPS 10
#define BUFFER_SIZE 256

typedef struct {
    char login[BUFFER_SIZE];
    char fifo_name[BUFFER_SIZE];
    int fd; 
} Client;

typedef struct {
    char name[BUFFER_SIZE];
    Client members[MAX_CLIENTS];
    int member_count;
} Group;

Client clients[MAX_CLIENTS];
int client_count = 0;

Group groups[MAX_GROUPS];
int group_count = 0;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void handle_signal(int sig) {
    printf("Server shutting down...\n");
    unlink("/tmp/chat_server");
    exit(0);
}

void send_to_group(const char* group_name, const char* sender, const char* message) {
    for (int i = 0; i < group_count; i++) {
        if (strcmp(groups[i].name, group_name) == 0) {
            for (int j = 0; j < groups[i].member_count; j++) {
                // Не отправляем сообщение отправителю
                if (strcmp(groups[i].members[j].login, sender) != 0) {
                    int client_fd = open(groups[i].members[j].fifo_name, O_WRONLY);
                    write(client_fd, message, strlen(message) + 1);
                    close(client_fd);
                }
            }
            break;
        }
    }
}

int main() {
    signal(SIGINT, handle_signal);

    // Создаем именованный канал для сервера
    mkfifo("/tmp/chat_server", 0666);
    int server_fd = open("/tmp/chat_server", O_RDONLY | O_NONBLOCK);

    printf("Server started. Waiting for clients...\n");

    while (1) {
        char buffer[BUFFER_SIZE];
        int bytes_read = read(server_fd, buffer, BUFFER_SIZE);

        if (bytes_read > 0) {
            char command[BUFFER_SIZE];
            char login[BUFFER_SIZE];
            char group_name[BUFFER_SIZE];
            char message[BUFFER_SIZE];
            sscanf(buffer, "%s %s %s %[^\n]", command, login, group_name, message);

            if (strcmp(command, "CONNECT") == 0) {
                // Новый клиент подключается
                printf("Client connected: %s\n", login);

                // Сохраняем информацию о клиенте
                pthread_mutex_lock(&mutex);
                strcpy(clients[client_count].login, login);
                sprintf(clients[client_count].fifo_name, "/tmp/chat_client_%s", login);
                clients[client_count].fd = open(clients[client_count].fifo_name, O_WRONLY); // Открываем FIFO для записи
                client_count++;
                pthread_mutex_unlock(&mutex);

                // Создаем именованный канал для клиента
                mkfifo(clients[client_count - 1].fifo_name, 0666);
            } else if (strcmp(command, "SEND") == 0) {
                // Клиент отправляет сообщение
                printf("Message from %s to %s: %s\n", login, group_name, message);

                pthread_mutex_lock(&mutex);
                for (int i = 0; i < client_count; i++) {
                    if (strcmp(clients[i].login, group_name) == 0) {
                        // Не отправляем сообщение отправителю
                        if (strcmp(clients[i].login, login) != 0) {
                            int client_fd = open(clients[i].fifo_name, O_WRONLY);
                            write(client_fd, message, strlen(message) + 1);
                            close(client_fd);
                        }
                    }
                }
                pthread_mutex_unlock(&mutex);
            } else if (strcmp(command, "CREATE_GROUP") == 0) {
                // Создание новой группы
                printf("Group created: %s by %s\n", group_name, login);

                pthread_mutex_lock(&mutex);
                strcpy(groups[group_count].name, group_name);
                group_count++;
                pthread_mutex_unlock(&mutex);
            } else if (strcmp(command, "JOIN_GROUP") == 0) {
                // Присоединение к группе
                pthread_mutex_lock(&mutex);
                for (int i = 0; i < group_count; i++) {
                    if (strcmp(groups[i].name, group_name) == 0) {
                        strcpy(groups[i].members[groups[i].member_count].login, login);
                        sprintf(groups[i].members[groups[i].member_count].fifo_name, "/tmp/chat_client_%s", login);
                        groups[i].member_count++;
                        printf("%s joined group %s\n", login, group_name);
                        break;
                    }
                }
                pthread_mutex_unlock(&mutex);
            } else if (strcmp(command, "SEND_GROUP") == 0) {
                // Отправка сообщения в группу
                printf("Group message from %s to %s: %s\n", login, group_name, message);

                pthread_mutex_lock(&mutex);
                send_to_group(group_name, login, message);
                pthread_mutex_unlock(&mutex);
            } else if (strcmp(command, "DISCONNECT") == 0) {
                // Удаление клиента
                printf("Client disconnected: %s\n", login);

                pthread_mutex_lock(&mutex);
                for (int i = 0; i < client_count; i++) {
                    if (strcmp(clients[i].login, login) == 0) {
                        close(clients[i].fd); // Закрываем файловый дескриптор
                        unlink(clients[i].fifo_name); // Удаляем FIFO
                        clients[i] = clients[client_count - 1];
                        client_count--;
                        break;
                    }
                }
                pthread_mutex_unlock(&mutex);
            }
        }

        usleep(100000); // Небольшая задержка для уменьшения нагрузки на CPU
    }

    close(server_fd);
    unlink("/tmp/chat_server");
    return 0;
}