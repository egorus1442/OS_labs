#include <stdio.h>

// Ряд Лейбница
float PiLeibniz(int K) {
    float pi = 0.0;
    for (int i = 0; i < K; i++) {
        pi += (i % 2 == 0 ? 1.0 : -1.0) / (2 * i + 1);
    }
    return 4 * pi;
}

// Формула Валлиса
float PiWallis(int K) {
    float pi = 1.0;
    for (int i = 1; i <= K; i++) {
        pi *= (4.0 * i * i) / (4.0 * i * i - 1);
    }
    return 2 * pi;
}