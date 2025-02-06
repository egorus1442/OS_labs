#include <stdio.h>

// Объявление функций из библиотек
extern int PrimeCountNaive(int, int);
extern float PiLeibniz(int);

int main() {
    int command;
    while (1) {
        printf("Input program code:\n");
        printf(" 1 -> PrimeCount\n");
        printf(" 2 -> Pi\n");
        printf("-1 -> Exit\n");
        scanf("%d", &command);

        if (command == -1) {
            break;
        } else if (command == 1) {
            int A, B;
            printf("Enter A and B: ");
            scanf("%d %d", &A, &B);
            printf("PrimeCount(%d, %d) = %d\n", A, B, PrimeCountNaive(A, B));
        } else if (command == 2) {
            int K;
            printf("Enter K: ");
            scanf("%d", &K);
            printf("Pi(%d) = %f\n", K, PiLeibniz(K));
        } else {
            printf("Invalid command\n");
        }
    }
    return 0;
}