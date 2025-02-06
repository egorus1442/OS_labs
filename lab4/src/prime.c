#include <stdio.h>
#include <math.h>

// Наивный алгоритм
int PrimeCountNaive(int A, int B) {
    int count = 0;
    for (int i = A; i <= B; i++) {
        if (i < 2) continue;
        int is_prime = 1;
        for (int j = 2; j <= sqrt(i); j++) {
            if (i % j == 0) {
                is_prime = 0;
                break;
            }
        }
        if (is_prime) count++;
    }
    return count;
}

// Решето Эратосфена
int PrimeCountEratosthenes(int A, int B) {
    if (B < 2) return 0;
    int size = B + 1;
    int sieve[size];
    for (int i = 0; i < size; i++) sieve[i] = 1;

    sieve[0] = sieve[1] = 0;
    for (int i = 2; i * i <= B; i++) {
        if (sieve[i]) {
            for (int j = i * i; j <= B; j += i) {
                sieve[j] = 0;
            }
        }
    }

    int count = 0;
    for (int i = A; i <= B; i++) {
        if (sieve[i]) count++;
    }
    return count;
}