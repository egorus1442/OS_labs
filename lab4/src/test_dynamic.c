#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>

int main() {
    void *prime_handle = dlopen("./libprime.so", RTLD_LAZY);
    void *pi_handle = dlopen("./libpi.so", RTLD_LAZY);

    if (!prime_handle || !pi_handle) {
        fprintf(stderr, "Ошибка загрузки библиотек: %s\n", dlerror());
        return 1;
    }

    // Загрузка функций
    int (*PrimeCountNaive)(int, int) = dlsym(prime_handle, "PrimeCountNaive");
    int (*PrimeCountEratosthenes)(int, int) = dlsym(prime_handle, "PrimeCountEratosthenes");
    float (*PiLeibniz)(int) = dlsym(pi_handle, "PiLeibniz");
    float (*PiWallis)(int) = dlsym(pi_handle, "PiWallis");

    if (!PrimeCountNaive || !PrimeCountEratosthenes || !PiLeibniz || !PiWallis) {
        fprintf(stderr, "Ошибка загрузки функций: %s\n", dlerror());
        dlclose(prime_handle);
        dlclose(pi_handle);
        return 1;
    }

    // Текущие реализации
    int (*CurrentPrimeCount)(int, int) = PrimeCountNaive;
    float (*CurrentPi)(int) = PiLeibniz;

    // Переменные для хранения текущей версии
    int prime_version = 1; // 1 - наивный, 2 - решето Эратосфена
    int pi_version = 1;    // 1 - Лейбниц, 2 - Валлис

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
            // Переключение реализаций
            prime_version = (prime_version == 1) ? 2 : 1;
            pi_version = (pi_version == 1) ? 2 : 1;

            CurrentPrimeCount = (prime_version == 1) ? PrimeCountNaive : PrimeCountEratosthenes;
            CurrentPi = (pi_version == 1) ? PiLeibniz : PiWallis;

            printf("Library switched successfully!\n");
            printf("Current PrimeCount implementation: %s\n", (prime_version == 1) ? "Naive" : "Eratosthenes");
            printf("Current Pi implementation: %s\n", (pi_version == 1) ? "Leibniz" : "Wallis");
        } else if (command == 1) {
            int A, B;
            printf("Enter A and B: ");
            scanf("%d %d", &A, &B);
            printf("PrimeCount(%d, %d) = %d\n", A, B, CurrentPrimeCount(A, B));
            printf("Implementation used: %s\n", (prime_version == 1) ? "Naive" : "Eratosthenes");
        } else if (command == 2) {
            int K;
            printf("Enter K: ");
            scanf("%d", &K);
            printf("Pi(%d) = %f\n", K, CurrentPi(K));
            printf("Implementation used: %s\n", (pi_version == 1) ? "Leibniz" : "Wallis");
        } else {
            printf("Invalid command\n");
        }
    }

    dlclose(prime_handle);
    dlclose(pi_handle);
    return 0;
}