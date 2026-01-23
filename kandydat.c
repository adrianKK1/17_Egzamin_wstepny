#include "common.h"
#include "utils.h"

int msgid = -1; 
int shmid = -1;
StudentWynik *baza_shm = NULL;
int moj_idx_shm = -1;

void znajdz_swoj_rekord(pid_t pid, int max) {
    for(int i=0; i<max; i++) {
        if (baza_shm[i].pid == pid) { moj_idx_shm = i; return; }
    }
}

// Funkcja czekająca aż w SHM pojawi się X pytań
void czekaj_na_pytania(int ile_wymaganych, char typ) {
    printf(ANSI_BROWN "[KANDYDAT %d] Czekam na %d pytań w SHM... " ANSI_RESET, getpid(), ile_wymaganych);
    fflush(stdout);
    
    while(1) {
        int obecne = (typ == 'A') ? baza_shm[moj_idx_shm].licznik_pytan_A : baza_shm[moj_idx_shm].licznik_pytan_B;
        if (obecne == ile_wymaganych) break;
        usleep(20000); 
    }
    printf("Gotowe!\n");
}

int main() {
    pid_t moj_pid = getpid();
    srand(time(NULL) ^ moj_pid); 
    int czy_poprawkowicz = (rand() % 100 < 2) ? 1 : 0;

    printf(ANSI_BROWN "[KANDYDAT %d] Start. Poprawkowicz: %s" ANSI_RESET "\n", moj_pid, czy_poprawkowicz?"TAK":"NIE");

    key_t key = ftok(PROG_SCIEZKA, PROG_ID);
    msgid = msgget(key, 0);
    int semid = semget(key, 4, 0);
    shmid = shmget(key, 0, 0); 
    baza_shm = (StudentWynik*)shmat(shmid, NULL, 0);
    znajdz_swoj_rekord(moj_pid, 2000);

    // ETAP 1: MATURA
    operacja_semafor(semid, SEM_DOSTEP_IDX, -1);
    Komunikat msg; 
    msg.mtype = MSG_MATURA_REQ; msg.nadawca_pid = moj_pid; msg.status_specjalny = czy_poprawkowicz;
    msgsnd(msgid, &msg, sizeof(Komunikat)-sizeof(long), 0);
    msgrcv(msgid, &msg, sizeof(Komunikat)-sizeof(long), moj_pid, 0);
    operacja_semafor(semid, SEM_DOSTEP_IDX, 1);

    if(msg.dane_int == 0) {
        printf(ANSI_RED "[KANDYDAT %d] Brak matury. Koniec." ANSI_RESET "\n", moj_pid);
        exit(0);
    }

    //  ETAP 2: KOMISJA A
    // A) Wejście
    printf(ANSI_BROWN "[KANDYDAT %d] Czekam na wejście do A..." ANSI_RESET "\n", moj_pid);
    operacja_semafor(semid, SEM_KOMISJA_A_IDX, -1); 
    
    msg.mtype = MSG_WEJSCIE_A; 
    msg.nadawca_pid = moj_pid;
    msg.status_specjalny = czy_poprawkowicz;
    msgsnd(msgid, &msg, sizeof(Komunikat)-sizeof(long), 0);

    
    // LOGIKA "SZYBKIEJ ŚCIEŻKI" DLA POPRAWKOWICZA
    if (czy_poprawkowicz) {
        printf(ANSI_GREEN "[KANDYDAT %d] Jestem poprawkowiczem - czekam na automatyczne zaliczenie." ANSI_RESET "\n", moj_pid);
        // Nie czeka na pytania, nie śpi, nie odpowiada
    } else {
        // --- ZWYKŁY STUDENT ---
        czekaj_na_pytania(5, 'A');

        printf(ANSI_BROWN "[KANDYDAT %d] Mam pytania. Mysle (Ti)..." ANSI_RESET "\n", moj_pid);
        usleep(losuj(500000, 900000)); 

        // Losowe odpowiedzi
        for(int i=0; i<5; i++) {
            baza_shm[moj_idx_shm].odpowiedzi_A[i] = losuj(1, 100); 
        }
        
        // Zmiana statusu w SHM (Sygnał dla egzaminatorów)
        baza_shm[moj_idx_shm].status_A = ODPOWIEDZI_GOTOWE_CZEKA_NA_OCENY;
        
        printf(ANSI_BROWN "[KANDYDAT %d] Odpowiedzi wpisane. Czekam na wynik." ANSI_RESET "\n", moj_pid);
    }

    // Wspólny odbiór wyniku dla obu typów
    msgrcv(msgid, &msg, sizeof(Komunikat)-sizeof(long), moj_pid, 0);
    int ocena_a = msg.dane_int;
    
    operacja_semafor(semid, SEM_KOMISJA_A_IDX, 1); 

    if (ocena_a < 30) {
        printf(ANSI_RED "[KANDYDAT %d] Wynik A: %d%%. Porażka." ANSI_RESET "\n", moj_pid, ocena_a);
        exit(0);
    }
    printf(ANSI_GREEN "[KANDYDAT %d] Wynik A: %d%%. Ide do B." ANSI_RESET "\n", moj_pid, ocena_a);


    // --- ETAP 3: KOMISJA B ---
    printf(ANSI_BROWN "[KANDYDAT %d] Czekam na wejście do B..." ANSI_RESET "\n", moj_pid);
    operacja_semafor(semid, SEM_KOMISJA_B_IDX, -1); 

    msg.mtype = MSG_WEJSCIE_B; 
    msg.nadawca_pid = moj_pid;
    msg.status_specjalny = 0; // W B już nie jest specjalny
    msgsnd(msgid, &msg, sizeof(Komunikat)-sizeof(long), 0);

    czekaj_na_pytania(3, 'B');

    printf(ANSI_BROWN "[KANDYDAT %d] Mam pytania B. Mysle..." ANSI_RESET "\n", moj_pid);
    usleep(losuj(500000, 900000));

    for(int i=0; i<3; i++) {
        baza_shm[moj_idx_shm].odpowiedzi_B[i] = losuj(1, 100);
    }
    baza_shm[moj_idx_shm].status_B = ODPOWIEDZI_GOTOWE_CZEKA_NA_OCENY;

    msgrcv(msgid, &msg, sizeof(Komunikat)-sizeof(long), moj_pid, 0);
    int ocena_b = msg.dane_int;

    operacja_semafor(semid, SEM_KOMISJA_B_IDX, 1);

    if (ocena_b >= 30) printf(ANSI_GREEN "[KANDYDAT %d] Wynik B: %d%%. Koniec." ANSI_RESET "\n", moj_pid, ocena_b);
    else printf(ANSI_RED "[KANDYDAT %d] Wynik B: %d%%. Porażka." ANSI_RESET "\n", moj_pid, ocena_b);

    return 0;
}