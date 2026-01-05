#ifndef UTILS_H
#define UTILS_H

// Funkcja sprawdzająca błędy funkcji systemowych
// val: wynik funkcji systemowej
// msg: komunikat do wyświetlenia w przypadku błędu
void check_error(int val, const char *msg);

// Funkcja losująca liczbę z zakresu [min, max]
int losuj(int min, int max);

// Funkcja do pobierania aktualnego czasu jako string (do logów)
char* aktualny_czas();

#endif