#include "common.h"
#include "utils.h"
#include <time.h>
#include <string.h>

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