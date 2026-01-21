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

// Funkcja sprawdzająca błędy funkcji systemowych
// val: wynik funkcji systemowej
// msg: komunikat do wyświetlenia w przypadku błędu
void check_error(int val, const char *msg);

// Funkcja losująca liczbę z zakresu [min, max]
int losuj(int min, int max);

// Funkcja do pobierania aktualnego czasu jako string (do logów)
char* aktualny_czas();

//Funkcja logująca (zapisuje czas, tekst na ekran i opcjonalnie do pliku)
void loguj(FILE* plik, const char *format, ...);

// Funkcja wykonująca operację na semaforze
void operacja_semafor(int semid, int sem_idx, int op);

#endif