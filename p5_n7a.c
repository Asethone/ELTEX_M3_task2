// Программа 1. Иллюстрация синхронизации процессов с использованием семафоров

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/sem.h>

int main() {
    int *array;             // Указатель на разделяемую память
    int shmid;              // IPC дескриптор для области разделяемой памяти
    int new = 1;            // Флаг необходимости инициализации элементов массива
    char pathname[] = "p5_n7a.c";   // Имя файла, использующееся для генерации ключа
    key_t key;              // IPC ключ
    struct sembuf mybuf;    // Структура операций над массивом семафоров
    int semid;              // IPC дескриптор семафоров
    // Для устранения мусора в неинициализированной структуре делаем следующее:
    mybuf.sem_flg = 0;  // Зануляем флаги
    mybuf.sem_num = 0;  // Используем только один (нулевой) семафор 
    // Генерация ключа
    if((key = ftok(pathname,0)) < 0) {
        printf("Can\'t generate key!\n");
        exit(-1);
    }
    // Cоздаем область разделяемой памяти (РП). Если РП уже существует, получаем к ней доступ и обнуляем флаг new
    if((shmid = shmget(key, 3*sizeof(int), 0666|IPC_CREAT|IPC_EXCL)) < 0){
        if(errno != EEXIST){
            printf("Can\'t create shared memory\n");
            exit(-1);
        } else {
            if((shmid = shmget(key, 3*sizeof(int), 0)) < 0){
                printf("Can\'t find shared memory\n");
                exit(-1);
            }
            new = 0;
        }
    }
    // Создание семафора и увеличение его на 1. Если семафор уже существует, не увеличиваем его на 1, а просто получаем его IPC-дескриптор
    semid = semget(key, 1, 0666 | IPC_CREAT | IPC_EXCL);
    if (semid >= 0) {
        mybuf.sem_op = 1;
        if (semop(semid, &mybuf, 1) < 0) {
            printf("Error increasing semaphore!\n");
            exit(-1);
        }
    } else {
        semid = semget(key, 1, 0);
    }
    // Отображаем РП в адрессное пространство процесса
    if((array = (int *)shmat(shmid, NULL, 0)) == (int *)(-1)){
        printf("Can't attach shared memory\n");
        exit(-1);
    }
    // Инициализируем элементы массива в РП, если флаг new = 1
    if(new){
        array[0] = 1;
        array[1] = 0;
        array[2] = 1;
    } else {    // Если инициализация не требуется (new = 0), то происходит следующее:
        // Уменьшение семафора на единицу и его захват (либо блокировка процесса)
        mybuf.sem_op = -1;
        if (semop(semid, &mybuf, 1) < 0) {
            printf("Error decreasing semaphore!\n");
            exit(-1);
        }
        // Имитация задержки (примерно 5 сек). Увеличение соответствующих элементов массива в РП 
        for (int i = 0; i < 1000000000; i++) {}
        array[0] += 1;
        array[2] += 1;
        // Увеличение семафора на 1
        mybuf.sem_op = 1;
        if (semop(semid, &mybuf, 1) < 0) {
            printf("Error increasing semaphore!\n");
            exit(-1);
        }
    }
    // Вывод данных о запуске программ на экран
    printf("Program 1 was spawn %d times, program 2 - %d times, total - %d times\n", array[0], array[1], array[2]);
    // Удаление РП из адресного пространства процесса
    if(shmdt(array) < 0){
        printf("Can't detach shared memory\n");
        exit(-1);
    }
    return 0;
}