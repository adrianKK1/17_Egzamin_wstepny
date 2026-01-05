#ifndef COMMON_H
#define COMMON_H

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

//stale projektowe
#define M_MIEJSC 120        // Liczba miejsc na roku
#define LIMIT_SALA 3        // Max osób w sali (wymóg tematu)
#define PROG_SCIEZKA "."    // Ścieżka do generowania klucza ftok
#define PROG_ID 'E'         // ID projektu do ftok

// Klucze do IPC (zostaną wygenerowane przez ftok)
// Użyjemy jednego klucza bazowego, a różne mechanizmy (sem, msg) pobiorą go sobie

// Typy komunikatów (mtype) - porządkują ruch w jednej kolejce
#define MSG_MATURA_REQ    1   // Kandydat -> Dziekan (Pytanie o maturę)
#define MSG_MATURA_RESP   2   // Dziekan -> Kandydat (Odpowiedź: TAK/NIE)
#define MSG_WEJSCIE_A     3   // Kandydat -> Komisja A
#define MSG_WYNIK_A       4   // Komisja A -> Kandydat + Dziekan
#define MSG_WEJSCIE_B     5   // Kandydat -> Komisja B
#define MSG_WYNIK_B       6   // Komisja B -> Kandydat + Dziekan
#define MSG_KONIEC        99  // Sygnał zakończenia

// Struktura komunikatu (wymóg: long mtype na początku) [cite: 762, 1367]
typedef struct {
    long mtype;             // Typ komunikatu
    pid_t nadawca_pid;      // PID procesu wysyłającego (żeby wiedzieć komu odpisać)
    int dane_int;           // Np. ocena, status (0/1)
    char tresc[256];        // Opcjonalny tekst (np. do raportu)
} Komunikat;

// Indeksy semaforów w zbiorze
#define SEM_KOMISJA_A_IDX 0 // Licznik wolnych miejsc w A
#define SEM_KOMISJA_B_IDX 1 // Licznik wolnych miejsc w B
#define SEM_START_IDX     2 // Semafor startowy (Dziekan podnosi o godzinie T)

#endif // COMMON_H
