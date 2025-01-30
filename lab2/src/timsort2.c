#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#define MIN_MERGE 32
#define MIN(a, b) ((a) < (b) ? (a) : (b))

typedef struct {
    int *array;
    int left;
    int right;
} ThreadData;

void insertionSort(int arr[], int left, int right) {
    for (int i = left + 1; i <= right; i++) {
        int temp = arr[i];
        int j = i - 1;
        while (j >= left && arr[j] > temp) {
            arr[j + 1] = arr[j];
            j--;
        }
        arr[j + 1] = temp;
    }
}

void merge(int arr[], int l, int m, int r) {
    int len1 = m - l + 1, len2 = r - m;
    int *left = (int *)malloc(len1 * sizeof(int));
    int *right = (int *)malloc(len2 * sizeof(int));

    for (int i = 0; i < len1; i++)
        left[i] = arr[l + i];
    for (int i = 0; i < len2; i++)
        right[i] = arr[m + 1 + i];

    int i = 0, j = 0, k = l;
    while (i < len1 && j < len2) {
        if (left[i] <= right[j])
            arr[k++] = left[i++];
        else
            arr[k++] = right[j++];
    }

    while (i < len1)
        arr[k++] = left[i++];

    while (j < len2)
        arr[k++] = right[j++];

    free(left);
    free(right);
}

void *timSortThread(void *arg) {
    ThreadData *data = (ThreadData *)arg;
    int *arr = data->array;
    int left = data->left;
    int right = data->right;

    int minRun = MIN_MERGE;
    for (int start = left; start <= right; start += minRun) {
        int end = MIN(start + minRun - 1, right);
        insertionSort(arr, start, end);
    }

    for (int size = minRun; size < right - left + 1; size = 2 * size) {
        for (int start = left; start <= right; start += 2 * size) {
            int mid = start + size - 1;
            int end = MIN(start + 2 * size - 1, right);
            if (mid < end) {
                merge(arr, start, mid, end);
            }
        }
    }

    pthread_exit(NULL);
}

void timSort(int arr[], int n, int maxThreads) {
    if (n < 2) return;

    pthread_t *threads = (pthread_t *)malloc(maxThreads * sizeof(pthread_t));
    ThreadData *threadData = (ThreadData *)malloc(maxThreads * sizeof(ThreadData));

    int chunkSize = (n + maxThreads - 1) / maxThreads;
    for (int i = 0; i < maxThreads; i++) {
        int start = i * chunkSize;
        int end = MIN(start + chunkSize - 1, n - 1);
        if (start >= n) break;

        threadData[i].array = arr;
        threadData[i].left = start;
        threadData[i].right = end;

        pthread_create(&threads[i], NULL, timSortThread, (void *)&threadData[i]);
    }

    for (int i = 0; i < maxThreads; i++) {
        pthread_join(threads[i], NULL);
    }

    // Слияние частей массива
    for (int size = chunkSize; size < n; size = 2 * size) {
        for (int left = 0; left < n; left += 2 * size) {
            int mid = left + size - 1;
            int right = MIN(left + 2 * size - 1, n - 1);
            if (mid < right) {
                merge(arr, left, mid, right);
            }
        }
    }

    free(threads);
    free(threadData);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <max_threads>\n", argv[0]);
        return 1;
    }

    int maxThreads = atoi(argv[1]);
    int arraySize;

    printf("Enter the number of elements in the array: ");
    scanf("%d", &arraySize);

    int *arr = (int *)malloc(arraySize * sizeof(int));

    // Инициализация генератора случайных чисел
    srand(time(NULL));

    // Генерация случайных чисел для массива
    for (int i = 0; i < arraySize; i++) {
        arr[i] = rand() % 100; // Генерация случайных чисел от 0 до 99
    }

    //printf("Original array:\n");
    //for (int i = 0; i < arraySize; i++) {
    //    printf("%d ", arr[i]);
    //}
    //printf("\n");

    struct timeval start, end;
    gettimeofday(&start, NULL);

    timSort(arr, arraySize, maxThreads);

    gettimeofday(&end, NULL);

    //printf("Sorted array:\n");
    //for (int i = 0; i < arraySize; i++) {
    //    printf("%d ", arr[i]);
    //}
    //printf("\n");

    long seconds = end.tv_sec - start.tv_sec;
    long microseconds = end.tv_usec - start.tv_usec;
    double elapsed = seconds + microseconds * 1e-6;

    printf("Time taken for sorting: %.6f seconds\n", elapsed);
    printf("Number of threads used: %d\n", maxThreads);

    free(arr);
    return 0;
}
