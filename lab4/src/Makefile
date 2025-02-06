# Компилятор
CC = gcc
# Флаги компиляции
CFLAGS = -fPIC -Wall -Wextra
# Флаги для создания динамических библиотек
LDFLAGS = -shared
# Флаги для линковки с библиотекой dl (для динамической загрузки)
DLFLAGS = -ldl

# Имена библиотек
LIB_PRIME = libprime.so
LIB_PI = libpi.so

# Имена тестовых программ
TEST_STATIC = test_static
TEST_DYNAMIC = test_dynamic

# Все цели по умолчанию
all: $(LIB_PRIME) $(LIB_PI) $(TEST_STATIC) $(TEST_DYNAMIC)

# Сборка динамической библиотеки для prime.c
$(LIB_PRIME): prime.o
	$(CC) $(LDFLAGS) -o $@ $^

# Сборка динамической библиотеки для pi.c
$(LIB_PI): pi.o
	$(CC) $(LDFLAGS) -o $@ $^

# Компиляция prime.c в объектный файл
prime.o: prime.c
	$(CC) $(CFLAGS) -c $< -o $@

# Компиляция pi.c в объектный файл
pi.o: pi.c
	$(CC) $(CFLAGS) -c $< -o $@

# Сборка тестовой программы №1 (статическое использование библиотек)
$(TEST_STATIC): test_static.c
	$(CC) -o $@ $< -L. -lprime -lpi

# Сборка тестовой программы №2 (динамическая загрузка библиотек)
$(TEST_DYNAMIC): test_dynamic.c
	$(CC) -o $@ $< $(DLFLAGS)

# Очистка сгенерированных файлов
clean:
	rm -f *.o *.so $(TEST_STATIC) $(TEST_DYNAMIC)

# Указание целей, которые не являются файлами
.PHONY: all clean