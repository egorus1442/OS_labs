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
    sem_t sem_data_ready;      // Семафор: данные готовы для обработки
    sem_t sem_data_processed_by_1;  // Семафор: данные обработаны
    sem_t sem_data_processed_by_2;  // Семафор: данные обработаны
} SharedData;