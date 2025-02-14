#include <stdio.h>

// Формула Валлиса
float CalculatePi(int K) {
    float pi = 1.0;
    for (int i = 1; i <= K; i++) {
        pi *= (4.0 * i * i) / (4.0 * i * i - 1);
    }
    return 2 * pi;
}