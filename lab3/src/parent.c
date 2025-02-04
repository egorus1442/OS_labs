#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>

#define MAX_LINE 1000
#define SHARED_FILE "shared_memory.bin"

typedef struct {
    char data[MAX_LINE];
    int ready;  // Флаг готовности данных: 0 - не готово, 1 - готово
    int done;   // Флаг завершения: 0 - работаем, 1 - завершаем
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

    // Инициализация shared memory
    shared->ready = 0;
    shared->done = 0;

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
        shared->ready = 1;  // Помечаем данные как готовые

        // Ждем, пока child1 обработает данные
        while (shared->ready == 1) {
            usleep(1000);  // Спим 1 мс, чтобы не нагружать CPU
        }

        // Ждем, пока child2 обработает данные
        while (shared->ready == 2) {
            usleep(1000);
        }

        // Выводим результат
        printf("Результат: %s\n", shared->data);
    }

    // Сигнализируем дочерним процессам о завершении
    shared->done = 1;

    // Ожидание завершения дочерних процессов
    wait(NULL);  // Ждем завершения child1
    wait(NULL);  // Ждем завершения child2

    // Освобождаем shared memory
    munmap(shared, sizeof(SharedData));

    printf("\nВсе процессы завершены.\n");

    return 0;
}