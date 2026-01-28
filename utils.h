#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>

// --- KOLORY ANSI ---
#define ANSI_RESET   "\x1b[0m"
#define ANSI_RED     "\x1b[31m"     // Dla błędów i odrzuconych
#define ANSI_GREEN   "\x1b[32m"     // Dla przyjętych i zdanej matury
#define ANSI_YELLOW  "\x1b[33m"     // Dla Komisji B
#define ANSI_BLUE    "\x1b[34m"     // Dla Komisji A
#define ANSI_MAGENTA "\x1b[35m"     // Dla Dziekana
#define ANSI_CYAN    "\x1b[36m"     // Dla Kandydatów
#define ANSI_BOLD    "\x1b[1m"      // Pogrubienie nagłówków
#define ANSI_BROWN   "\x1b[38;5;94m"   // Ciemny Brązowy dla Kandydata
#define ANSI_ORANGE  "\x1b[38;5;208m"  // Soczysty Pomarańczowy dla Komisji B

/* Sprawdza wynik wywołania funkcji systemowej i kończy program przy błędzie */
void check_error(int val, const char *msg);

/* Zwraca losową liczbę całkowitą z zakresu [min, max] */
int losuj(int min, int max);

/* Zwraca aktualny czas w formie tekstowej (do logów) */
char* aktualny_czas();

/* Logowanie zdarzeń: na ekran i opcjonalnie do pliku */
void loguj(FILE* plik, const char *format, ...);

/* Wykonuje operację P/V na wskazanym semaforze */
void operacja_semafor(int semid, int sem_idx, int op);

#endif