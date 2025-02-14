#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>

typedef int (*tCalculatePrime)(int, int);
typedef float (*tCalculatePi)(int);

typedef struct {
    tCalculatePi calculatePi;
    tCalculatePrime calculatePrime;
    void* library;
} tFuncLibrary;

tFuncLibrary load_library(char* filename) {
    // Загрузка библиотеки
    tFuncLibrary result;
    result.library = dlopen(filename, RTLD_LAZY); // RTLD_LAZY - отложенная загрузка
    if (!result.library) {
        fprintf(stderr, "Ошибка загрузки библиотек: %s\n", dlerror());
        return result;
    }

    // Загрузка функций
    result.calculatePrime = dlsym(result.library, "CalculatePrime");
    result.calculatePi = dlsym(result.library, "CalculatePi");

    if (!result.calculatePrime || !result.calculatePi) { //не возвращают ли NULL
        fprintf(stderr, "Ошибка загрузки функций из библиотеки: %s\n", dlerror());
        dlclose(result.library);
        result.library = NULL;
        return result;
    }

    return result;
}

int main() {
    tFuncLibrary funcLib = load_library("./libImpl1.so");
    if (funcLib.library == NULL) {
        return 1;
    }
    int lib_index = 0;

    int command;
    while (1) {
        printf("Input program code:\n");
        printf(" 0 -> Library switch\n");
        printf(" 1 -> PrimeCount\n");
        printf(" 2 -> Pi\n");
        printf("-1 -> Exit\n");
        scanf("%d", &command);

        if (command == -1) {
            break;
        } else if (command == 0) {
            dlclose(funcLib.library);
            lib_index = lib_index == 0 ? 1 : 0;
            funcLib = load_library(lib_index == 0 ? "./libImpl1.so" : "./libImpl2.so");
            if (funcLib.library == NULL) {
                continue;
            }

            printf("Library switched successfully!\n");
            printf("Current lib: %s\n", lib_index == 0 ? "./libImpl1.so" : "./libImpl2.so");
        } else if (command == 1) {
            int A, B;
            printf("Enter A and B: ");
            scanf("%d %d", &A, &B);
            printf("PrimeCount(%d, %d) = %d\n", A, B, funcLib.calculatePrime(A, B));
            printf("Implementation used: %s\n", lib_index == 0 ? "./libImpl1.so" : "./libImpl2.so");
        } else if (command == 2) {
            int K;
            printf("Enter K: ");
            scanf("%d", &K);
            printf("Pi(%d) = %f\n", K, funcLib.calculatePi(K));
            printf("Implementation used: %s\n", lib_index == 0 ? "./libImpl1.so" : "./libImpl2.so");
        } else {
            printf("Invalid command\n");
        }
    }

    dlclose(funcLib.library);
    return 0;
}