#include <stdio.h>

// Ряд Лейбница
float CalculatePi(int K) {
    float pi = 0.0;
    for (int i = 0; i < K; i++) {
        pi += (i % 2 == 0 ? 1.0 : -1.0) / (2 * i + 1);
    }
    return 4 * pi;
}