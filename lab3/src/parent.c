#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>

#define MAX_LINE 1000
#define SHARED_FILE "shared_memory.bin"

typedef struct {
    char data[MAX_LINE];
    sem_t sem_data_ready;      // Семафор: данные готовы для обработки
    sem_t sem_data_processed;  // Семафор: данные обработаны
    sem_t sem_done;            // Семафор: работа завершена
    sem_t sem_child1_done;     // Семафор: child1 завершил работу
    sem_t sem_child2_done;     // Семафор: child2 завершил работу
} SharedData;

int main() {
    int fd;
    SharedData *shared;
    pid_t child1, child2;

    // Создаем файл для shared memory
    fd = open(SHARED_FILE, O_RDWR | O_CREAT | O_TRUNC, 0666);
    if (fd == -1) {
        perror("Ошибка открытия файла");
        exit(1);
    }

    // Устанавливаем размер файла
    if (ftruncate(fd, sizeof(SharedData)) == -1) {
        perror("Ошибка ftruncate");
        close(fd);
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

    // Инициализация семафоров
    sem_init(&shared->sem_data_ready, 1, 0);      // Изначально данные не готовы
    sem_init(&shared->sem_data_processed, 1, 0);  // Изначально данные не обработаны
    sem_init(&shared->sem_done, 1, 0);            // Изначально работа не завершена
    sem_init(&shared->sem_child1_done, 1, 0);     // Изначально child1 не завершил работу
    sem_init(&shared->sem_child2_done, 1, 0);     // Изначально child2 не завершил работу

    // Создаем первый дочерний процесс
    if ((child1 = fork()) == -1) {
        perror("Ошибка создания первого дочернего процесса");
        munmap(shared, sizeof(SharedData));
        exit(1);
    }

    if (child1 == 0) {
        // Код для первого дочернего процесса (child1)
        execl("./child1", "child1", SHARED_FILE, NULL);
        perror("Ошибка execl для child1");
        exit(1);
    }

    // Создаем второй дочерний процесс
    if ((child2 = fork()) == -1) {
        perror("Ошибка создания второго дочернего процесса");
        munmap(shared, sizeof(SharedData));
        exit(1);
    }

    if (child2 == 0) {
        // Код для второго дочернего процесса (child2)
        execl("./child2", "child2", SHARED_FILE, NULL);
        perror("Ошибка execl для child2");
        exit(1);
    }

    // Родительский процесс
    printf("Введите строки (Ctrl+D для завершения):\n");

    while (fgets(shared->data, MAX_LINE, stdin) != NULL) {
        // Сигнализируем, что данные готовы для обработки
        sem_post(&shared->sem_data_ready);

        // Ждем, пока данные будут обработаны
        sem_wait(&shared->sem_data_processed);

        // Выводим результат
        printf("Результат: %s\n", shared->data);
    }

    // Сигнализируем дочерним процессам о завершении
    sem_post(&shared->sem_done);

    // Ожидание завершения дочерних процессов
    sem_wait(&shared->sem_child1_done);  // Ждем завершения child1
    sem_wait(&shared->sem_child2_done);  // Ждем завершения child2

    // Уничтожение семафоров
    sem_destroy(&shared->sem_data_ready);
    sem_destroy(&shared->sem_data_processed);
    sem_destroy(&shared->sem_done);
    sem_destroy(&shared->sem_child1_done);
    sem_destroy(&shared->sem_child2_done);

    // Освобождаем shared memory
    munmap(shared, sizeof(SharedData));

    printf("\nВсе процессы завершены.\n");

    return 0;
}
