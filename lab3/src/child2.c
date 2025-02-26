#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>
#include "common.h"

void remove_extra_spaces(char *str) {
    int i = 0, j = 0;
    int space_found = 0;

    while (str[i]) {
        if (str[i] == ' ') {
            if (!space_found) {
                str[j++] = str[i];
                space_found = 1;
            }
        } else {
            str[j++] = str[i];
            space_found = 0;
        }
        i++;
    }
    str[j] = '\0';
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Не указан файл shared memory\n");
        exit(1);
    }

    const char *shared_file = argv[1];
    int fd;
    SharedData *shared;

    // Открываем файл shared memory
    fd = open(shared_file, O_RDWR);
    if (fd == -1) {
        perror("Ошибка открытия файла");
        exit(1);
    }

    // Отображаем файл в память
    shared = (SharedData *)mmap(NULL, sizeof(SharedData), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (shared == MAP_FAILED) {
        perror("Ошибка mmap");
        close(fd);
        exit(1);
    }

    close(fd);  // Файловый дескриптор больше не нужен

    while (1) {
        // Ждем, пока данные будут готовы для обработки
        sem_wait(&shared->sem_data_processed_by_1);

        // Проверяем, завершена ли работа
        if (shared->data[0] == '\0') {
            sem_post(&shared->sem_data_processed_by_2);
            break;
        }

        // Удаляем лишние пробелы
        remove_extra_spaces(shared->data);

        // Сигнализируем, что данные обработаны
        sem_post(&shared->sem_data_processed_by_2);
    }

    // Освобождаем shared memory
    munmap(shared, sizeof(SharedData));

    return 0;
}
