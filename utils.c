#include "common.h"
#include "utils.h"
#include <time.h>
#include <string.h>
#include <stdarg.h> // Potrzebne do va_list
#include <sys/sem.h> // Potrzebne do sembuf

void check_error(int val, const char *msg) {
    if (val == -1) {
        perror(msg); // Wypisuje błąd systemowy (z errno) [cite: 161, 351]
        exit(EXIT_FAILURE);
    }
}

int losuj(int min, int max) {
    return min + rand() % (max - min + 1);
}

// Prosty helper do logowania czasu
char* aktualny_czas() {
    static char bufor[30];
    time_t now;
    time(&now);
    struct tm *t = localtime(&now);
    strftime(bufor, sizeof(bufor), "%H:%M:%S", t);
    return bufor;
}

//funkcja zapisujaca logi do pliku i na ekran
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

//funkcje pomocnicze do operacji na semgaforach (P - czrekaj, V - sygnalizuj)
void operacja_semafor(int semid,int sem_idx, int op){
    struct sembuf bufor; 
    bufor.sem_num = sem_idx; //indeks semafora w zbiorze
    bufor.sem_op = op; //-1 czekaj/zajmiij, lub 1 zwolnij
    bufor.sem_flg = 0; //brak flag specjalnych

    //jesli semafor jest zajety, proces zostanie zablokowany tutaj
    if (semop(semid, &bufor, 1) == -1) {
        //igonorujemy blad przerwania przez dziekana
        if (errno != EINTR && errno != EIDRM) {
            perror("[KOMISJA] Blad operacji na semaforze");
            exit(1);
        } else {
            exit(0); 
        }
    }
}