#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>

#define MAX_LINE 1000

void remove_extra_spaces(char *str) {
    int i = 0, j = 0;
    int space_found = 0;

    while (str[i]) {
        if (str[i] == ' ') {
            if (!space_found) {
                str[j++] = str[i];
                space_found = 1;
            }
        } else {
            str[j++] = str[i];
            space_found = 0;
        }
        i++;
    }
    str[j] = '\0';
}

int main() {
    char line[MAX_LINE];

    // Чтение строк из stdin (перенаправленный pipe2)
    while (read(STDIN_FILENO, line, MAX_LINE) != 0) {
        // Удаление лишних пробелов
        remove_extra_spaces(line);
        // Вывод результата в stdout (будет прочитан родительским процессом)
        write(STDOUT_FILENO, line, strlen(line));
    }

    return 0;
}

