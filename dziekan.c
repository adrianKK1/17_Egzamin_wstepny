// Wklej to do dziekan.c, komisja.c i kandydat.c (tymczasowo) test test
#include "common.h"
#include "utils.h"
#include <signal.h>

//zmienne globalne
int msgid = -1;
int semid = -1;

//definicja unii dla semctl
union semun {
    int val;                // wartosc dla SETVAL
    struct semid_ds *buf;   // bufor dla IPC_STAT, IPC_SET
    unsigned short *array;  // tablica dla GETALL, SETALL
    struct seminfo *__buf;  // bufor dla IPC_INFO (Linux specyficzne)
};

//funkcja czyszcząca zasoby IPC
void sprzatanie() {
    printf("\n[Dziekan] Rozpoczynam sprzatanie zasobow IPC...\n");
    if(semid != -1) {
        if(semctl(semid, 0, IPC_RMID) == -1) {
            perror("[Dziekan] Blad przy usuwaniu semafora");
        } else {
            printf("[Dziekan] Semafor usuniety pomyslnie.\n");
        }
    }   
    if(msgid != -1) {
        if(msgctl(msgid, IPC_RMID, NULL) == -1) {
            perror("[Dziekan] Blad przy usuwaniu kolejki komunikatow");
        } else {
            printf("[Dziekan] Kolejka komunikatow usunieta pomyslnie.\n");
        }
    }
}

//handler sygnalu SIGINT (ctrl+c)
void obsluga_sigint(int sig) {
    (void)sig; // unikniecie ostrzezenia o nieuzywanej zmiennej
    printf("\n[Dziekan] Otrzymano sygnal SIGINT. Zamykanie procesu...\n");
    sprzatanie();
    exit(0);
}


int main() {
    //1.obsuluga sygnalu SIGINT
    // Jeśli naciśniesz Ctrl+C, wykona się funkcja obsluga_sigint 
    signal(SIGINT, obsluga_sigint);

    //2. generowanie klucza IPC
    // Funkcja ftok tworzy unikalny klucz na podstawie ścieżki i znaku 
    key_t key = ftok(PROG_SCIEZKA, PROG_ID);
    check_error(key, "[DZIEKAN] Blad ftok - czy plik common.h istnieje w tej sciezce?");
    printf("[DZIEKAN] Wygenerowano klucz IPC: %d\n", key);

    // 3. Tworzenie Kolejki Komunikatów
    // msgget tworzy kolejkę. 0600 to prawa rw------- (tylko dla właściciela - wymóg minimalnych praw 4.1.d)
    // IPC_CREAT tworzy nową, jeśli nie istnieje 
    msgid = msgget(key, IPC_CREAT | 0600);
    check_error(msgid, "[DZIEKAN] Blad tworzenia kolejki komunikatow");
    printf("[DZIEKAN] Utworzono kolejke komunikatow ID: %d\n", msgid);

    // 4.Tworzenie Semaforow
    // Tworzymy zbiór 3 semaforów (zgodnie z definicjami w common.h) 
    semid = semget(key, 3, IPC_CREAT | 0600);
    check_error(semid, "[DZIEKAN] Blad tworzenia zbioru semaforow");
    printf("[DZIEKAN] Utworzono zbior semaforow ID: %d\n", semid);
    
    // 5. Inicjalizacja Semaforów
    // Używamy semctl z komendą SETVAL, aby ustawić wartości początkowe 
    union semun arg;
    
    // A) Limit miejsc w Komisji A (3 miejsca)
    arg.val = LIMIT_SALA;
    check_error(semctl(semid, SEM_KOMISJA_A_IDX, SETVAL, arg), 
                "[DZIEKAN] Błąd inicjalizacji semafora Komisji A");

    // B) Limit miejsc w Komisji B (3 miejsca)
    arg.val = LIMIT_SALA;
    check_error(semctl(semid, SEM_KOMISJA_B_IDX, SETVAL, arg), 
                "[DZIEKAN] Błąd inicjalizacji semafora Komisji B");

    // C) Semafor startowy (0 - blokada kandydatów do godziny T)
    arg.val = 0;
    check_error(semctl(semid, SEM_START_IDX, SETVAL, arg), 
                "[DZIEKAN] Błąd inicjalizacji semafora Startowego");

    printf("[DZIEKAN] Semafory zainicjalizowane (A=%d, B=%d, START=0).\n", LIMIT_SALA, LIMIT_SALA);
    printf("[DZIEKAN] System gotowy. Naciśnij Ctrl+C aby zakończyć i posprzątać.\n");

    while (1) {
        sleep(1);
    }

    return 0;
}