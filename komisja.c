#include "common.h"
#include "utils.h"
#include <pthread.h>

//zmienne globalne dla wątków
int msgid = -1;
int shmid = -1;
char typ_komisji; // 'A' lub 'B'
int liczba_egzaminatorow; 
int liczba_pytan_wymagana;
int max_studentow = 0;
StudentWynik *baza_shm = NULL;
const char* KOM_COLOR = ANSI_RESET;

// --- STOLIKI ---
// Komisja ma 3 stoliki. Każdy stolik przechowuje indeks studenta w SHM.
// -1 oznacza stolik wolny.
int stoliki[LIMIT_SALA]; 
pthread_mutex_t mutex_stoliki[LIMIT_SALA]; // Ochrona dostępu do danego stolika

// --- WĄTEK RECEPCJI (PRZEWODNICZĄCY) ---
// Jego zadaniem jest tylko wpuszczać ludzi na wolne stoliki
void* watek_recepcji(void* arg) {
    (void)arg;
    long typ_wejscia = (typ_komisji == 'A') ? MSG_WEJSCIE_A : MSG_WEJSCIE_B;
    Komunikat msg;

    printf("%s[Recepcja %c] Otwieram zapisy. Czekam na kandydatow.%s\n", KOM_COLOR, typ_komisji, ANSI_RESET);

    while(1) {
        // Odbierz zgłoszenie
        if (msgrcv(msgid, &msg, sizeof(Komunikat) - sizeof(long), typ_wejscia, 0) == -1) {
            if (errno == EIDRM) break; // Kolejka usunięta
            continue;
        }

        // Znajdź studenta w SHM (zeby znac jego index)
        int idx_shm = -1;
        for(int i=0; i<max_studentow; i++) {
            if (baza_shm[i].pid == msg.nadawca_pid) { idx_shm = i; break; }
        }

        if (idx_shm == -1) {
            printf(ANSI_RED "BLAD: Nieznany student PID %d!\n" ANSI_RESET, msg.nadawca_pid);
            continue;
        }

        // --- OBSŁUGA POPRAWKOWICZA (TYLKO KOMISJA A) ---
        // "jeśli nim jest to informuje tylko komisje A... i automatycznie przechodzi... z wynikiem 100%"
        if (typ_komisji == 'A' && msg.status_specjalny == 1) {
            printf("%s[Recepcja A] PID %d to POPRAWKOWICZ. Zaliczam automatycznie (100%%).%s\n", 
                   ANSI_BLUE, msg.nadawca_pid, ANSI_RESET);
            
            // Wpisz wynik do SHM
            baza_shm[idx_shm].ocena_koncowa_A = 100;
            
            // Odeślij natychmiast wynik
            Komunikat wyn;
            wyn.mtype = msg.nadawca_pid;
            wyn.dane_int = 100;
            msgsnd(msgid, &wyn, sizeof(Komunikat)-sizeof(long), 0);
            
            continue; // Pomiń przydzielanie stolika!
        }

        // Znajdź wolny stolik
        int znaleziono_stolik = 0;
        while(!znaleziono_stolik) {
            for(int i=0; i<LIMIT_SALA; i++) {
                pthread_mutex_lock(&mutex_stoliki[i]);
                if (stoliki[i] == -1) { // Wolne!
                    stoliki[i] = idx_shm;
                    
                    // Inicjalizacja stanu w SHM dla tego etapu
                    if (typ_komisji == 'A') {
                        baza_shm[idx_shm].licznik_pytan_A = 0;
                        baza_shm[idx_shm].licznik_ocen_A = 0;
                        baza_shm[idx_shm].status_A = ZAJETE_CZEKA_NA_PYTANIA;
                        // Reset tablic
                        for(int k=0; k<5; k++) baza_shm[idx_shm].id_egzaminatora_A[k] = 0;
                        for(int k=0; k<5; k++) baza_shm[idx_shm].oceny_A[k] = 0; // Wyzerowanie ocen
                    } else {
                        baza_shm[idx_shm].licznik_pytan_B = 0;
                        baza_shm[idx_shm].licznik_ocen_B = 0;
                        baza_shm[idx_shm].status_B = ZAJETE_CZEKA_NA_PYTANIA;
                        for(int k=0; k<3; k++) baza_shm[idx_shm].id_egzaminatora_B[k] = 0;
                        for(int k=0; k<3; k++) baza_shm[idx_shm].oceny_B[k] = 0;
                    }

                    printf("%s[Recepcja %c] Student PID %d zajal stolik %d.%s\n", 
                           KOM_COLOR, typ_komisji, msg.nadawca_pid, i+1, ANSI_RESET);
                    
                    znaleziono_stolik = 1;
                }
                pthread_mutex_unlock(&mutex_stoliki[i]);
                if (znaleziono_stolik) break;
            }
            if (!znaleziono_stolik) usleep(10000); // Czekaj na wolne miejsce
        }
    }
    return NULL;
}

// --- WĄTEK ROBOCZY (EGZAMINATOR) ---
// Biega po stolikach i szuka pracy
void* watek_egzaminatora(void* arg) {
    int id_egzaminatora = *((int*)arg);
    free(arg);
    
    printf("%s[Egzaminator %c-%d] Gotowy do pracy.%s\n", KOM_COLOR, typ_komisji, id_egzaminatora, ANSI_RESET);

    while(1) {
        int zrobilem_cos = 0;

        // Pętla po stolikach (Work Stealing)
        for (int i=0; i<LIMIT_SALA; i++) {
            pthread_mutex_lock(&mutex_stoliki[i]);
            int idx = stoliki[i];
            
            if (idx != -1) { // Jest student
                
                // Wskaźniki na odpowiednie pola
                int *licznik_pytan, *licznik_ocen, *status;
                int *pytania, *oceny, *ids;
                int *ocena_koncowa;

                if (typ_komisji == 'A') {
                    licznik_pytan = &baza_shm[idx].licznik_pytan_A;
                    licznik_ocen = &baza_shm[idx].licznik_ocen_A;
                    status = &baza_shm[idx].status_A;
                    pytania = baza_shm[idx].pytania_A;
                    oceny = baza_shm[idx].oceny_A;
                    ids = baza_shm[idx].id_egzaminatora_A;
                    ocena_koncowa = &baza_shm[idx].ocena_koncowa_A;
                } else {
                    licznik_pytan = &baza_shm[idx].licznik_pytan_B;
                    licznik_ocen = &baza_shm[idx].licznik_ocen_B;
                    status = &baza_shm[idx].status_B;
                    pytania = baza_shm[idx].pytania_B;
                    oceny = baza_shm[idx].oceny_B;
                    ids = baza_shm[idx].id_egzaminatora_B;
                    ocena_koncowa = &baza_shm[idx].ocena_koncowa_B;
                }

                // --- ZADANIE 1: ZADAWANIE PYTAŃ ---
                if (*status == ZAJETE_CZEKA_NA_PYTANIA && *licznik_pytan < liczba_pytan_wymagana) {
                    // Sprawdź czy ja już zadałem pytanie temu studentowi
                    int ja_juz_zadalem = 0;
                    for(int k=0; k < *licznik_pytan; k++) {
                        if (ids[k] == id_egzaminatora) ja_juz_zadalem = 1;
                    }

                    if (!ja_juz_zadalem) {
                        // ZADAJE PYTANIE
                        int nr_pytania = *licznik_pytan;
                        pytania[nr_pytania] = losuj(100, 999);
                        ids[nr_pytania] = id_egzaminatora;
                        (*licznik_pytan)++; // Atomowo (mutex stolika trzyma)
                        
                        printf("%s[Egzaminator %c-%d] Zadaje pytanie %d/%d (Tresc: %d) studentowi PID %d.%s\n",
                               KOM_COLOR, typ_komisji, id_egzaminatora, nr_pytania+1, liczba_pytan_wymagana, pytania[nr_pytania], baza_shm[idx].pid, ANSI_RESET);
                        
                        zrobilem_cos = 1;

                        // Czy to było ostatnie pytanie?
                        if (*licznik_pytan == liczba_pytan_wymagana) {
                             *status = PYTANIA_GOTOWE_CZEKA_NA_KANDYDATA;
                             printf("%s[Komisja %c] Wszystkie pytania gotowe dla PID %d. Czekamy na odpowiedzi.%s\n",
                                    KOM_COLOR, typ_komisji, baza_shm[idx].pid, ANSI_RESET);
                        }
                    }
                }

                // --- USUNIĘTO OBSŁUGĘ TIMEOUTU (Zgodnie z poleceniem) ---
                
                // --- ZADANIE 3: OCENIANIE ---
                else if (*status == ODPOWIEDZI_GOTOWE_CZEKA_NA_OCENY && *licznik_ocen < liczba_pytan_wymagana) {
                     // Szukam czy jest tu MOJE pytanie, ktorego jeszcze nie ocenilem
                     // Pytanie "k" zadał egzaminator "ids[k]". Jeśli to ja I ocena[k] jest 0 -> oceniam.
                     
                     for (int k=0; k < liczba_pytan_wymagana; k++) {
                         if (ids[k] == id_egzaminatora && oceny[k] == 0) {
                             // OCENIAM LOSOWO (Zgodnie z poleceniem: "ocena za każdą odpowiedź jest losowana")
                             oceny[k] = losuj(0, 100);

                             printf("%s[Egzaminator %c-%d] Oceniam odp studenta PID %d. Pyt: %d -> Ocena: %d%%%s\n",
                                    KOM_COLOR, typ_komisji, id_egzaminatora, baza_shm[idx].pid, pytania[k], oceny[k], ANSI_RESET);
                             
                             (*licznik_ocen)++;
                             zrobilem_cos = 1;
                             break; // Jedna akcja na cykl pętli
                         }
                     }

                     // Czy wszystko ocenione?
                     if (*licznik_ocen == liczba_pytan_wymagana) {
                         *status = OCENIONE_GOTOWE_DO_WYSYLKI;
                         
                         // Oblicz średnią (Rola Przewodniczącego)
                         int suma = 0;
                         for(int k=0; k<liczba_pytan_wymagana; k++) suma += oceny[k];
                         *ocena_koncowa = suma / liczba_pytan_wymagana;
                         
                         printf("%s[Komisja %c] PID %d - Koniec oceniania. Srednia: %d%%. Zwalniam stolik.%s\n",
                                KOM_COLOR, typ_komisji, baza_shm[idx].pid, *ocena_koncowa, ANSI_RESET);

                         // Wyślij wynik kandydatowi (mtype = PID)
                         Komunikat wyn;
                         wyn.mtype = baza_shm[idx].pid; 
                         wyn.dane_int = *ocena_koncowa;
                         msgsnd(msgid, &wyn, sizeof(Komunikat)-sizeof(long), 0);

                         // Zwolnij stolik
                         stoliki[i] = -1;
                     }
                }
            } // koniec if idx != -1
            pthread_mutex_unlock(&mutex_stoliki[i]);
        } // koniec for stoliki

        if (!zrobilem_cos) usleep(10000); 
        else usleep(5000); 
    }
    return NULL;
}


int main(int argc, char* argv[]) {
    if (argc != 3){ fprintf(stderr, "Uzycie: %s <typ> <max>\n", argv[0]); exit(1); }
    typ_komisji = argv[1][0];
    max_studentow = atoi(argv[2]);

    if (typ_komisji == 'A'){
        liczba_egzaminatorow = 5;
        liczba_pytan_wymagana = 5;
        KOM_COLOR = ANSI_BLUE;   
    } else {
        liczba_egzaminatorow = 3;
        liczba_pytan_wymagana = 3;
        KOM_COLOR = ANSI_ORANGE;
    }

    // Init Stoliki
    for(int i=0; i<LIMIT_SALA; i++) {
        stoliki[i] = -1;
        pthread_mutex_init(&mutex_stoliki[i], NULL);
    }

    // IPC
    key_t key = ftok(PROG_SCIEZKA, PROG_ID);
    msgid = msgget(key, 0);
    shmid = shmget(key, sizeof(StudentWynik) * max_studentow, 0); 
    baza_shm = (StudentWynik*)shmat(shmid, NULL, 0);

    printf("%s[Komisja %c] Start Puli Watkow (%d egzaminatorow na 3 stoliki).%s\n", KOM_COLOR, typ_komisji, liczba_egzaminatorow, ANSI_RESET);

    // 1. Wątek Recepcji
    pthread_t t_recepcja;
    pthread_create(&t_recepcja, NULL, watek_recepcji, NULL);

    // 2. Wątki Egzaminatorów (Pula)
    pthread_t* t_egzam = malloc(sizeof(pthread_t) * liczba_egzaminatorow);
    for(int i=0; i<liczba_egzaminatorow; i++) {
        int* id = malloc(sizeof(int)); *id = i+1;
        pthread_create(&t_egzam[i], NULL, watek_egzaminatora, id);
    }

    pthread_join(t_recepcja, NULL); // Main czeka na recepcję
    return 0;
}