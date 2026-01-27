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
void czekaj_na_pytania(pid_t moj_pid, int ile_wymaganych) {
    printf(ANSI_BROWN "[KANDYDAT %d] Czekam na %d pytań w SHM... " ANSI_RESET, getpid(), ile_wymaganych);
    fflush(stdout);

    Komunikat msg;
    ssize_t result; 

    // Pętla odporna na sygnały (Ctrl+Z / fg)
    while (1) {
        result = msgrcv(msgid, &msg, sizeof(Komunikat)-sizeof(long), moj_pid, 0);
        
        if (result == -1) {
            if (errno == EINTR) continue; // To tylko pauza/wznowienie, wracamy do czekania
            
            if (errno == EIDRM || errno == EINVAL) {
                // Kolejka usunięta przez Dziekana -> koniec
                printf(ANSI_RED "\n[KANDYDAT %d] Ewakuacja (kolejka usunieta).\n" ANSI_RESET, moj_pid);
                exit(0);
            }
            perror("Blad msgrcv w kandydata");
            exit(1);
        }
        // Jeśli udało się odebrać (result != -1), wychodzimy z pętli
        break;
    }
    
    // Sprawdzenie czy to właściwy kod (dla bezpieczeństwa)
    if (msg.dane_int == CODE_PYTANIA_GOTOWE) {
        printf("Gotowe! Otrzymalem pytania.\n");
    } else {
        // Jeśli dostaliśmy coś innego (np. jakiś błędny status), traktujemy to jako błąd logiczny
        printf(ANSI_RED "Blad: Otrzymano nieznany kod %d zamiast pytań.\n" ANSI_RESET, msg.dane_int);
        exit(1);
    }
}

int main(int argc, char* argv[]) {

    if (argc < 2) {
        fprintf(stderr, "Kandydat: brak argumentu indeksu w wywolaniu execl\n");
        exit(1);
    }
    moj_idx_shm = atoi(argv[1]);

    pid_t moj_pid = getpid();
    srand(time(NULL) ^ moj_pid); 
    int czy_poprawkowicz = (rand() % 100 < 2) ? 1 : 0;

    printf(ANSI_BROWN "[KANDYDAT %d] Start. Poprawkowicz: %s" ANSI_RESET "\n", moj_pid, czy_poprawkowicz?"TAK":"NIE");

    key_t key = ftok(PROG_SCIEZKA, PROG_ID);
    msgid = msgget(key, 0);
    int semid = semget(key, 4, 0);
    shmid = shmget(key, 0, 0); 
    baza_shm = (StudentWynik*)shmat(shmid, NULL, 0);
    if (baza_shm == (void*)-1) { perror("Błąd shmat"); exit(1); }
    if (baza_shm[moj_idx_shm].pid == -1 || baza_shm[moj_idx_shm].pid == 0) {
        baza_shm[moj_idx_shm].pid = moj_pid;
    }
    //znajdz_swoj_rekord(moj_pid, 2000);

    // ETAP 1: MATURA
    operacja_semafor(semid, SEM_DOSTEP_IDX, -1);
    Komunikat msg; 
    msg.mtype = MSG_MATURA_REQ; msg.nadawca_pid = moj_pid; msg.status_specjalny = czy_poprawkowicz;
    msgsnd(msgid, &msg, sizeof(Komunikat)-sizeof(long), 0);

    ssize_t res;
    while(1) {
        res = msgrcv(msgid, &msg, sizeof(Komunikat)-sizeof(long), moj_pid, 0);
        if (res == -1) {
            if (errno == EINTR) continue; // Restart po sygnale
            // Inny błąd to prawdopodobnie koniec symulacji
            printf(ANSI_RED "[KANDYDAT %d] Ewakuacja podczas matury.\n" ANSI_RESET, moj_pid); 
            exit(0); 
        }
        break;
    }

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
        czekaj_na_pytania(moj_pid,5);

        printf(ANSI_BROWN "[KANDYDAT %d] Mam pytania. Mysle (Ti)..." ANSI_RESET "\n", moj_pid);
        usleep(losuj(500000, 900000)); 

        operacja_semafor(semid, SEM_DOSTEP_IDX, -1);

        // Losowe odpowiedzi
        for(int i=0; i<5; i++) {
            baza_shm[moj_idx_shm].odpowiedzi_A[i] = losuj(1, 100); 
        }
        
        // Zmiana statusu w SHM (Sygnał dla egzaminatorów)
        baza_shm[moj_idx_shm].status_A = ODPOWIEDZI_GOTOWE_CZEKA_NA_OCENY;

        operacja_semafor(semid, SEM_DOSTEP_IDX, 1);
        
        printf(ANSI_BROWN "[KANDYDAT %d] Odpowiedzi wpisane. Czekam na wynik." ANSI_RESET "\n", moj_pid);
    }


    // Wspólny odbiór wyniku dla obu typów
    while(1) {
        res = msgrcv(msgid, &msg, sizeof(Komunikat)-sizeof(long), moj_pid, 0);
        if (res == -1) {
            if (errno == EINTR) continue;
            exit(0);
        }
        break;
    }

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

    czekaj_na_pytania(moj_pid,3);

    printf(ANSI_BROWN "[KANDYDAT %d] Mam pytania B. Mysle..." ANSI_RESET "\n", moj_pid);
    usleep(losuj(500000, 900000));

    operacja_semafor(semid, SEM_DOSTEP_IDX, -1);

    for(int i=0; i<3; i++) {
        baza_shm[moj_idx_shm].odpowiedzi_B[i] = losuj(1, 100);
    }
    baza_shm[moj_idx_shm].status_B = ODPOWIEDZI_GOTOWE_CZEKA_NA_OCENY;

    operacja_semafor(semid, SEM_DOSTEP_IDX, 1);

    while(1) {
        res = msgrcv(msgid, &msg, sizeof(Komunikat)-sizeof(long), moj_pid, 0);
        if (res == -1) {
            if (errno == EINTR) continue;
            exit(0);
        }
        break;
    }
    int ocena_b = msg.dane_int;

    operacja_semafor(semid, SEM_KOMISJA_B_IDX, 1);

    if (ocena_b >= 30) printf(ANSI_GREEN "[KANDYDAT %d] Wynik B: %d%%. Koniec." ANSI_RESET "\n", moj_pid, ocena_b);
    else printf(ANSI_RED "[KANDYDAT %d] Wynik B: %d%%. Porażka." ANSI_RESET "\n", moj_pid, ocena_b);

    return 0;
}