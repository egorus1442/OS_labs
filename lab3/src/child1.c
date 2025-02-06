#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>

#define MAX_LINE 1000

typedef struct {
    char data[MAX_LINE];
    sem_t sem_data_ready;
    sem_t sem_data_processed;
    sem_t sem_done;
} SharedData;

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
        sem_wait(&shared->sem_data_ready);

        // Проверяем, завершена ли работа
        if (sem_trywait(&shared->sem_done) == 0) {
            break;
        }

        // Преобразуем строку в верхний регистр
        for (int i = 0; shared->data[i]; i++) {
            shared->data[i] = toupper(shared->data[i]);
        }

        // Сигнализируем, что данные обработаны
        sem_post(&shared->sem_data_processed);
    }

    // Освобождаем shared memory
    munmap(shared, sizeof(SharedData));

    return 0;
}