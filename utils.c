#include "common.h"
#include "utils.h"
#include <time.h>
#include <string.h>
#include <stdarg.h>
#include <sys/sem.h> 

/* Sprawdza wynik wywołania funkcji systemowej */
void check_error(int val, const char *msg) {
    if (val == -1) {
        perror(msg);
        exit(EXIT_FAILURE);
    }
}

/* Losuje liczbę całkowitą z zakresu [min, max] */
int losuj(int min, int max) {
    return min + rand() % (max - min + 1);
}

/* Zwraca aktualny czas w formacie HH:MM:SS */
char* aktualny_czas() {
    static char bufor[30];
    time_t now;
    time(&now);
    struct tm *t = localtime(&now);
    strftime(bufor, sizeof(bufor), "%H:%M:%S", t);
    return bufor;
}

/* Logowanie zdarzeń: kolor na ekranie + zapis do pliku */
void loguj(FILE* plik, const char *format, ...) {
    va_list args;
    char bufor[1024];
    
    va_start(args, format);
    vsnprintf(bufor, sizeof(bufor), format, args);
    va_end(args);

    char* czas = aktualny_czas();

    const char* kolor = ANSI_RESET;
    if (strstr(bufor, "[Dziekan]")) kolor = ANSI_MAGENTA;
    else if (strstr(bufor, "Komisja A") || strstr(bufor, "KOMISJA A")) kolor = ANSI_BLUE;
    else if (strstr(bufor, "Komisja B") || strstr(bufor, "KOMISJA B")) kolor = ANSI_ORANGE;
    else if (strstr(bufor, "Kandydat") || strstr(bufor, "KANDYDAT")) kolor = ANSI_BROWN;
    else if (strstr(bufor, "BLAD") || strstr(bufor, "ODRZUCONY")) kolor = ANSI_RED;
    else if (strstr(bufor, "PRZYJETY")) kolor = ANSI_GREEN;

    printf("%s[%s] %s%s\n", kolor, czas, bufor, ANSI_RESET);

    if (plik) {
        fprintf(plik, "[%s] %s\n", czas, bufor);
        fflush(plik); 
    }
}

/* Operacja P/V na semaforze z obsługą przerwań i ewakuacji */
void operacja_semafor(int semid,int sem_idx, int op){
    struct sembuf bufor; 
    bufor.sem_num = sem_idx; 
    bufor.sem_op = op; 
    bufor.sem_flg = 0; 

    while (semop(semid, &bufor, 1) == -1) {
        if (errno == EINTR) {
            continue;
        } 
        else if (errno == EIDRM || errno == EINVAL) {
            exit(0);
        }
        else {
            perror("[KOMISJA/UTILS] Blad operacji na semaforze");
            exit(1);
        }
    }
}