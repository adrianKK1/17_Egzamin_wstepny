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
#include <time.h>
#include <stdarg.h>
#include <stdbool.h>

//stale projektowe
#define M_MIEJSC 120        // Liczba miejsc na roku
#define LIMIT_SALA 3        // Max osób w sali (wymóg tematu)
#define PROG_SCIEZKA "."    // Ścieżka do generowania klucza ftok
#define PROG_ID 'E'         // Identyfikator projektu (ftok)
#define SHM_ID 'S'          // Identyfikator SHM

// Klucze do IPC (zostaną wygenerowane przez ftok)
// Użyjemy jednego klucza bazowego, a różne mechanizmy (sem, msg) pobiorą go sobie

// --- TYPY KOMUNIKATÓW ---
#define MSG_MATURA_REQ      1  // Kandydat -> Dziekan (weryfikacja matury)

#define MSG_WEJSCIE_A       3   // Kandydat zgłasza chęć wejścia

#define MSG_WEJSCIE_B       6   // Kandydat -> Komisja B


#define CODE_PYTANIA_GOTOWE -999  // Sygnał od komisji: pytania są w SHM


enum StatusEgzaminu {
    WOLNE = 0,
    ZAJETE_CZEKA_NA_PYTANIA = 1,
    PYTANIA_GOTOWE_CZEKA_NA_KANDYDATA = 2,
    ODPOWIEDZI_GOTOWE_CZEKA_NA_OCENY = 3,
    OCENIONE_GOTOWE_DO_WYSYLKI = 4
};

// Struktura komunikatu 
typedef struct {
    long mtype;             // Typ komunikatu
    pid_t nadawca_pid;      // PID procesu wysyłającego (żeby wiedzieć komu odpisać)
    int dane_int;           // Np. ocena, status 
    int status_specjalny;   // 0=zwykły, 1=poprawkowicz
} Komunikat;

typedef struct {
    int id;
    pid_t pid;
    int matura_zdana; 
    // KOMISJA A 
    int pytania_A[5];          // ID pytania 
    int odpowiedzi_A[5];       // Treść odpowiedzi (liczba)
    int oceny_A[5];            // Oceny cząstkowe (0-100)
    int id_egzaminatora_A[5];  // ID egzaminatora, który zadał pytanie
    
    int licznik_pytan_A;       
    int licznik_ocen_A;        
    int status_A;              
    int ocena_koncowa_A;

    //  KOMISJA B 
    int pytania_B[3];
    int odpowiedzi_B[3];
    int oceny_B[3];
    int id_egzaminatora_B[3];
    
    int licznik_pytan_B;
    int licznik_ocen_B;
    int status_B;
    int ocena_koncowa_B;
  
    int suma_ocen;
} StudentWynik;

// Indeksy semaforów w zbiorze
#define SEM_KOMISJA_A_IDX 0 // Licznik wolnych miejsc w A
#define SEM_KOMISJA_B_IDX 1 // Licznik wolnych miejsc w B
#define SEM_START_IDX     2 // Semafor startowy (Dziekan podnosi o godzinie T)
#define SEM_DOSTEP_IDX    3 // Dostęp do sekcji krytycznych

#endif 
