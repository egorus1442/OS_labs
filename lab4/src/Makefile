# Компилятор
CC = gcc
# Флаги компиляции
CFLAGS = -fPIC -Wall -Wextra
# Флаги для создания динамических библиотек
LDFLAGS = -shared
# Флаги для линковки с библиотекой dl (для динамической загрузки)
DLFLAGS = -ldl

# Имена библиотек
LIB_1 = libImpl1.so
LIB_2 = libImpl2.so

# Имена тестовых программ
TEST_STATIC = test_static
TEST_DYNAMIC = test_dynamic

# Все цели по умолчанию
all: $(LIB_1) $(LIB_2) $(TEST_STATIC) $(TEST_DYNAMIC)

# Сборка динамической библиотеки для lib1
$(LIB_1): prime_naive.o pi_leibniz.o
	$(CC) $(LDFLAGS) -o $@ $^ -lm

# Сборка динамической библиотеки для lib2
$(LIB_2): prime_eratosthenus.o pi_wallis.o
	$(CC) $(LDFLAGS) -o $@ $^ -lm

# Компиляция первой библиотеки
prime_naive.o: prime_naive.c
	$(CC) $(CFLAGS) -c $< -o $@

pi_leibniz.o: pi_leibniz.c
	$(CC) $(CFLAGS) -c $< -o $@

# Компиляция второй библиотеки
prime_eratosthenus.o: prime_eratosthenus.c
	$(CC) $(CFLAGS) -c $< -o $@

pi_wallis.o: pi_wallis.c
	$(CC) $(CFLAGS) -c $< -o $@

# Сборка тестовой программы №1 (статическое использование библиотек)
$(TEST_STATIC): test_static.c
	$(CC) -o $@ $< -L. -lImpl1 -lm -Wl,-rpath,.

# Сборка тестовой программы №2 (динамическая загрузка библиотек)
$(TEST_DYNAMIC): test_dynamic.c
	$(CC) -o $@ $< $(DLFLAGS)

# Очистка сгенерированных файлов
clean:
	rm -f *.o *.so $(TEST_STATIC) $(TEST_DYNAMIC)

# Указание целей, которые не являются файлами
.PHONY: all clean