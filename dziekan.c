#include "common.h"
#include "utils.h"
#include <signal.h>
#include <time.h>
#include <stdarg.h>
#include <sys/wait.h>
#include <string.h>

//zmienne globalne
int msgid = -1;
int semid = -1;
int shmid = -1;
pid_t pid_komisja_A = -1;
pid_t pid_komisja_B = -1;
pid_t *pids_kandydatow = NULL;
int liczba_kandydatow = 0;
int liczba_miejsc = 0;
FILE *plik_raportu = NULL;

/* Wspólna baza wyników w pamięci dzielonej */
StudentWynik *baza_wynikow = NULL;

/* Unia wymagana przez semctl */
union semun {
    int val;                // wartosc dla SETVAL
    struct semid_ds *buf;   // bufor dla IPC_STAT, IPC_SET
    unsigned short *array;  // tablica dla GETALL, SETALL
};

//FUNKCJE POMOCNICZE

/* Szuka studenta po PID.
   Jeśli nie istnieje, dodaje go w pierwsze wolne miejsce. */
int znajdz_lub_dodaj_studenta(pid_t pid) {
    for (int i = 0; i < liczba_kandydatow; i++) {
        if (baza_wynikow[i].pid == pid) {
            return i;
        }
    }
    for (int i = 0; i < liczba_kandydatow; i++) {
        if (baza_wynikow[i].pid == 0) {
            baza_wynikow[i].pid = pid;
            baza_wynikow[i].id = i + 1;
            baza_wynikow[i].matura_zdana = -1;
            baza_wynikow[i].ocena_koncowa_A = -1;
            baza_wynikow[i].ocena_koncowa_B = -1;
            baza_wynikow[i].suma_ocen = 0;
            return i;
        }
    }
    return -1; 
}

/* Drukuje listy po weryfikacji matur:
   - niedopuszczeni
   - dopuszczeni */
void drukuj_listy_startowe() {
    loguj(plik_raportu,"\n[Dziekan] ZAKONCZONO WERYFIKACJE MATUR. PUBLIKACJA LIST STARTOWYCH.");

    if (plik_raportu) {
        fprintf(plik_raportu, "\n===LISTA NIEDOPUSZCZONYCH (BRAK MATURY) ===\n");
    }
    printf(ANSI_BOLD "\n--- LISTA OSOB NIEDOPUSZCZONYCH (BRAK MATURY) ---\n" ANSI_RESET);

    int brak_matury_count = 0;
    for (int i = 0; i < liczba_kandydatow; i++) {
        if(baza_wynikow[i].matura_zdana == 0) {
            char bufor[100];
            snprintf(bufor, sizeof(bufor), "Kandydat nr %d (PID: %d) - BRAK MATURY", baza_wynikow[i].id, baza_wynikow[i].pid);
            printf("%s\n", bufor);
            if (plik_raportu) {
                fprintf(plik_raportu, "%s\n", bufor);
                brak_matury_count++;
            }
        }
    }
    if (brak_matury_count == 0) {
            printf(ANSI_MAGENTA "BRAK OSOB NIEDOPUSZCZONYCH" ANSI_RESET "\n");
            if (plik_raportu) {
                fprintf(plik_raportu, "BRAK OSOB NIEDOPUSZCZONYCH\n");
            }
        }

    if (plik_raportu) {
        fprintf(plik_raportu, "\n===LISTA DOPUSZCZONYCH DO EGZAMINU ===\n");
    }
    printf(ANSI_BOLD "\n--- LISTA OSOB DOPUSZCZONYCH DO EGZAMINU ---\n" ANSI_RESET);

    int dopuszczeni_count = 0;
        for (int i = 0; i < liczba_kandydatow; i++) {
            if(baza_wynikow[i].matura_zdana == 1) {

                if (plik_raportu) {
                    fprintf(plik_raportu, "Student nr %d (PID: %d) - DOPUSZCZONY\n", baza_wynikow[i].id, baza_wynikow[i].pid);
                    dopuszczeni_count++;
                }
            }
        }
        printf(ANSI_MAGENTA "Statystyki wstepne: Dopuszczeni: %d, Odrzuceni: %d." ANSI_RESET "\n", dopuszczeni_count, brak_matury_count);
        loguj(plik_raportu,"[Dziekan] Statystyki wstepne: Miejsc: %d, Kandydatow: %d, Dopuszczeni: %d. Start egzaminow.\n", liczba_miejsc, liczba_kandydatow, dopuszczeni_count);
        if (plik_raportu) fprintf(plik_raportu, "--------------------------------------------------\n\n");

}

/* Porównanie do sortowania rankingu */
int porownaj_studentow(const void *a, const void *b) {
    StudentWynik *s1 = (StudentWynik *)a;
    StudentWynik *s2 = (StudentWynik *)b;

    int s1_zaliczyl = (s1->pid != 0 && s1->matura_zdana == 1 && s1->ocena_koncowa_A >= 30 && s1->ocena_koncowa_B >= 30);
    int s2_zaliczyl = (s2->pid != 0 && s2->matura_zdana == 1 && s2->ocena_koncowa_A >= 30 && s2->ocena_koncowa_B >= 30);

    if (s1_zaliczyl && !s2_zaliczyl) return -1;
    if (!s1_zaliczyl && s2_zaliczyl) return 1;
    if (s1_zaliczyl && s2_zaliczyl) {
        return s2->suma_ocen - s1->suma_ocen; 
    }
    return s1->id - s2->id;
    
}

/* Generuje końcowy ranking i listę przyjętych */
void generuj_ranking() {
    loguj(plik_raportu,"\n [Dziekan] KONIEC EGZAMINU. OBLICZANIE RANKINGU I PUBLIKACJA WYNIKOW");

    /* Obliczenie sum punktów */
    for (int i = 0; i < liczba_kandydatow; i++) {
        if (baza_wynikow[i].pid != 0) {
           int pkt_A = (baza_wynikow[i].ocena_koncowa_A == -1) ? 0 : baza_wynikow[i].ocena_koncowa_A;
           int pkt_B = (baza_wynikow[i].ocena_koncowa_B == -1) ? 0 : baza_wynikow[i].ocena_koncowa_B;
           baza_wynikow[i].suma_ocen = pkt_A + pkt_B;
        }
    }
    /* Sortowanie rankingu */
    qsort(baza_wynikow, liczba_kandydatow, sizeof(StudentWynik), porownaj_studentow);

    if (plik_raportu) {
        fprintf(plik_raportu, "\n=================================================================================\n");
        fprintf(plik_raportu, "| %-4s | %-10s | %-8s | %-8s | %-8s | %-5s | %-24s |\n", "NR", "PID", "MATURA", "KOM A", "KOM B", "SUMA", "STATUS");
        fprintf(plik_raportu, "=================================================================================\n");
    }

    printf("\n==== LISTA RANKINGOWA (Limit miejsc: %d) =====\n", liczba_miejsc);
    printf(ANSI_BOLD"| %-4s | %-10s | %-8s | %-8s | %-8s | %-5s | %-24s |\n", "NR", "PID", "MATURA", "OCENA A", "OCENA B", "SUMA", "STATUS" ANSI_RESET "\n");
    
    int przyjetych_count = 0;

    for (int i = 0; i < liczba_kandydatow; i++) {
        if (baza_wynikow[i].pid == 0) continue; 

        StudentWynik s = baza_wynikow[i];
        char status[50];
        char s_matura[20];
        char s_ocenaA[10];
        char s_ocenaB[10];

        // Opis Matury
        if(s.matura_zdana == 1) strcpy(s_matura, "OK");
        else if(s.matura_zdana == 0) strcpy(s_matura, "BRAK");
        else strcpy(s_matura, "---");
        
        // Opis Oceny A
        if (s.ocena_koncowa_A == -1){
            strcpy(s_ocenaA, "---");
        } else {
            sprintf(s_ocenaA, "%d%%", s.ocena_koncowa_A);
        }

        // Opis Oceny B
        if (s.ocena_koncowa_B == -1){
            strcpy(s_ocenaB, "---");
        } else {
            sprintf(s_ocenaB, "%d%%", s.ocena_koncowa_B);
        }

        //status końcowy
        int czy_zdal = (s.matura_zdana == 1 && s.ocena_koncowa_A >= 30 && s.ocena_koncowa_B >= 30 && s.ocena_koncowa_B != -1);
        
        if (s.matura_zdana == 0) {
            // 1 Odrzucony przez brak matury
            strcpy(status, "ODRZUCONY - BRAK MATURY");
        } 
        else if (s.ocena_koncowa_A != -1 && s.ocena_koncowa_A < 30) { 
            // 2 Oblal egzamin A (ma ocene i jest < 30)
            strcpy(status, "NIEZDAL (KOMISJA A)");
        } 
        else if (s.ocena_koncowa_B != -1 && s.ocena_koncowa_B < 30) {
            // 3 Oblal egzamin B (ma ocene i jest < 30)
            strcpy(status, "NIEZDAL (KOMISJA B)");  
        } 
        else if (czy_zdal) {
            //zdal ale sprawdzmy czy sa miejsca
            if (przyjetych_count < liczba_miejsc) {
                // 4 przyjety
                strcpy(status, "PRZYJETY");
                przyjetych_count++;
            } else {
                //przekroczono limit miejsc
                strcpy(status, "ODRZUCONY - BRAK MIEJSC");
            }
        } else {
            // 5 ewakuacja
            strcpy(status, "PRZERWANY - EWAKUACJA");
        }

        // Dobieramy kolor wiersza w zaleznosci od statusu
        const char* kolor_wiersza = ANSI_RESET;
        if (strcmp(status, "PRZYJETY") == 0) {
            kolor_wiersza = ANSI_GREEN;
        } else if (strstr(status, "ODRZUCONY") || strstr(status, "NIEZDAL") || strstr(status, "BRAK")) {
            kolor_wiersza = ANSI_RED;
        }

        // Wypisanie
        printf("%s| %-4d | %-10d | %-8s | %-8s | %-8s | %-5d | %-24s |\n%s", kolor_wiersza, s.id, s.pid, s_matura, s_ocenaA, s_ocenaB, s.suma_ocen, status, ANSI_RESET);
        if (plik_raportu) {
            fprintf(plik_raportu, "| %-4d | %-10d | %-8s | %-8s | %-8s | %-5d | %-24s |\n", s.id, s.pid, s_matura, s_ocenaA, s_ocenaB, s.suma_ocen, status);
        }
    }
    if (plik_raportu) fprintf(plik_raportu, "=================================================================================\n");
    printf("\n[Dziekan] Rekrutacja zakonczona. Zapelniono %d z %d miejsc.\n", przyjetych_count, liczba_miejsc);

    //drukowanie listy przyjetych
    if (plik_raportu) {
        fprintf(plik_raportu, "\n\n=================================================================================\n");
        fprintf(plik_raportu, "                          LISTA PRZYJETYCH                                          \n");
        fprintf(plik_raportu, "=================================================================================\n");
        fprintf(plik_raportu, "| %-4s | %-10s | %-8s | %-8s | %-12s |\n", "NR", "PID", "OCENA A", "OCENA B", "OCENA KONCOWA");
        fprintf(plik_raportu, "---------------------------------------------------------------------------------\n");
    }
    printf(ANSI_BOLD"\n\n====  LISTA PRZYJETYCH  ====" ANSI_RESET "\n");
    printf(ANSI_BOLD "| %-4s | %-10s | %-8s | %-8s | %-12s |\n", "NR", "PID", "OCENA A", "OCENA B", "OCENA KONCOWA" ANSI_RESET "\n");

    przyjetych_count = 0; 
    for (int i = 0; i < liczba_kandydatow; i++) {
        if (baza_wynikow[i].pid == 0) continue;
        
        StudentWynik s = baza_wynikow[i];
        
        // Warunek zaliczenia
        int czy_zdal = (s.matura_zdana == 1 && s.ocena_koncowa_A >= 30 && s.ocena_koncowa_B >= 30);
        
        if (czy_zdal) {
            if (przyjetych_count < liczba_miejsc) {
                // To jest osoba przyjeta - wypisujemy ja na drugiej liscie
                char s_ocenaA[10], s_ocenaB[10];
                sprintf(s_ocenaA, "%d%%", s.ocena_koncowa_A);
                sprintf(s_ocenaB, "%d%%", s.ocena_koncowa_B);

                printf(ANSI_GREEN "| %-4d | %-10d | %-8s | %-8s | %-13d |" ANSI_RESET "\n", s.id, s.pid, s_ocenaA, s_ocenaB, s.suma_ocen);
                if (plik_raportu) {
                    fprintf(plik_raportu, "| %-4d | %-10d | %-8s | %-8s | %-13d |\n", s.id, s.pid, s_ocenaA, s_ocenaB, s.suma_ocen);
                }
                przyjetych_count++;
            } else {
                break;
            }
        }
    }
    if (plik_raportu) fprintf(plik_raportu, "=================================================================================\n");

}



//funkcja czyszcząca zasoby IPC
void sprzatanie() {
    loguj(plik_raportu,"[Dziekan] Zamykanie systemu i sprzatanie zasobow IPC...");

    // Zabicie procesów komisji
    if (pid_komisja_A > 0) {
        printf("[Dziekan] Zamykam komisje A (PID: %d)...\n", pid_komisja_A);
        kill(pid_komisja_A, SIGTERM);
        waitpid(pid_komisja_A, NULL, 0);
    }
    if (pid_komisja_B > 0) {
        printf("[Dziekan] Zamykam komisje B (PID: %d)...\n", pid_komisja_B);
        kill(pid_komisja_B, SIGTERM);
        waitpid(pid_komisja_B, NULL, 0);
    }

    //zabijanie kandydatow
    if(pids_kandydatow) {
        for (int i = 0; i < liczba_kandydatow; i++) {
            if (pids_kandydatow[i] > 0) {
                kill(pids_kandydatow[i], SIGTERM);
            }
        }
        free(pids_kandydatow);
    }

    while (wait(NULL) > 0);

    if (baza_wynikow) {
        shmdt(baza_wynikow);
    }
    if (shmid != -1) {
        shmctl(shmid, IPC_RMID, NULL);
        printf("[Dziekan] Usunieto pamiec dzielona.\n");
    }

    //usuwanie ipc
    if (semid != -1) {
        semctl(semid, 0, IPC_RMID);
    }
    if (msgid != -1) {
        msgctl(msgid, IPC_RMID, NULL);
    }

    //zamkniecie pliku raportu
    if (plik_raportu) {
        fclose(plik_raportu);
    }

    printf("[Dziekan] Sprzatanie zakonczone. Do widzenia!\n");
}

//handler sygnalu SIGINT (ctrl+c)
void obsluga_sigint(int sig) {
    (void)sig; // unikniecie ostrzezenia o nieuzywanej zmiennej
    printf(ANSI_RED"\n[Dziekan] Otrzymano sygnal EWAKUACJA. Generowanie rankingu oraz zamkniecie uczelni..." ANSI_RESET "\n");
    generuj_ranking();
    sprzatanie();
    exit(0);
}


int main(int argc, char *argv[]) { 
    pid_t pid; 

        if (argc != 2) {
        fprintf(stderr, "Uzycie: %s <liczba_miejsc>\n", argv[0]);
        fprintf(stderr, "Przyklad: %s 120\n", argv[0]);
        exit(1);
    }

    int M_miejsc = atoi(argv[1]);
    
    // Sprawdzenie zakresu
    if (M_miejsc <= 0 || M_miejsc > 10000) {
        fprintf(stderr, "BLAD: Liczba miejsc musi byc w zakresie 1-10000\n");
        fprintf(stderr, "Podano: %d\n", M_miejsc);
        exit(1);
    }

    int ilosc_chetnych = M_miejsc * 10;
    
    // Ostrzeżenie przy dużej liczbie
    if (M_miejsc > 1000) {
        printf(ANSI_YELLOW "UWAGA: Duża liczba kandydatów (%d). " 
               "Symulacja może trwać długo.\n" ANSI_RESET, ilosc_chetnych);
    }

    liczba_kandydatow = ilosc_chetnych;
    liczba_miejsc = M_miejsc;

    //raport otwarcie pliku
    plik_raportu = fopen("raport.txt", "w");
    if (!plik_raportu) {
        perror("Nie mozna otworzyc pliku raportu");
        exit(1);
    }

    signal(SIGINT, obsluga_sigint); 

    //1 inicjalizacja IPC
    key_t key = ftok(PROG_SCIEZKA, PROG_ID);
    msgid = msgget(key, IPC_CREAT | 0600);
    semid = semget(key, 4, IPC_CREAT | 0600);

    //inicjalizacja pamieci dzielonej na baze wynikow
    shmid = shmget(key, sizeof(StudentWynik) * liczba_kandydatow, IPC_CREAT | 0600);
    if (shmid == -1) { 
        perror("Blad tworzenia SHM"); exit(1); 
    }
    baza_wynikow = (StudentWynik*)shmat(shmid, NULL, 0);
    if (baza_wynikow == (void*)-1) {
         perror("Blad shmat"); exit(1); 
    }
    memset(baza_wynikow, 0, sizeof(StudentWynik) * liczba_kandydatow);
    // Inicjalizacja wartosci domyslnych
    for (int i = 0; i<liczba_kandydatow; i++) {
        baza_wynikow[i].id = i + 1; 
        baza_wynikow[i].pid = 0;    
        baza_wynikow[i].matura_zdana = -1;
        baza_wynikow[i].ocena_koncowa_A = -1;
        baza_wynikow[i].ocena_koncowa_B = -1;
        baza_wynikow[i].suma_ocen = 0;
    }

    if (msgid == -1 || semid == -1) {
        perror("Blad inicjalizacji IPC");
        exit(1);
    }

    //2 inicjalizacja semaforow
    union semun arg;
    arg.val = LIMIT_SALA;
    semctl(semid, SEM_KOMISJA_A_IDX, SETVAL, arg);
    semctl(semid, SEM_KOMISJA_B_IDX, SETVAL, arg);
    arg.val = 0;
    semctl(semid, SEM_START_IDX, SETVAL, arg);
    arg.val = 50; 
    semctl(semid, SEM_DOSTEP_IDX, SETVAL, arg);
    
    loguj(plik_raportu,"[Dziekan] Uczelnia otwarta. Miejsc: %d. Przewidywana liczba kandydatow: %d.", liczba_miejsc, liczba_kandydatow);
   
    char buf_liczba[20];
    sprintf(buf_liczba, "%d", liczba_kandydatow);
    // Uruchomienie komisji A
    if ((pid_komisja_A = fork()) == -1) {
        perror("Blad fork() dla komisji A");
        sprzatanie();
        exit(1);
    }
    if (pid_komisja_A == 0) {
        execl("./komisja", "komisja", "A", buf_liczba, NULL); 
        perror("Blad execl() komisji A"); 
        exit(1);
    }

    // Uruchomienie komisji B
    if ((pid_komisja_B = fork()) == -1) {
        perror("Blad fork() dla komisji B");
        kill(pid_komisja_A, SIGTERM);
        sprzatanie();
        exit(1);
    }
    if (pid_komisja_B == 0) {
        execl("./komisja", "komisja", "B", buf_liczba, NULL); 
        perror("Blad execl() komisji B"); 
        exit(1);
    }

    loguj(plik_raportu,"[Dziekan] Komisja A (PID: %d) i komisja B (PID: %d) rozpoczynaja prace.", pid_komisja_A, pid_komisja_B);
    sleep(1);

    //3 uruchomienie kandydatow
    pids_kandydatow = malloc(sizeof(pid_t) * liczba_kandydatow);

    for (int i = 0; i < liczba_kandydatow; i++) {
        baza_wynikow[i].pid = -1;
        pid = fork(); 
        if (pid == -1) {
        perror("Blad fork() dla kandydata");
        // Nie przerywaj, kontynuuj z mniejszą liczbą
        liczba_kandydatow = i; 
        break;
    }
    
    if (pid == 0) {
        char arg_idx[16];
        sprintf(arg_idx, "%d", i);
        execl("./kandydat", "kandydat", arg_idx, NULL);
        perror("Blad execl() kandydata");
        exit(1);
    }
        
        // Zapisanie danych kandydata w bazie OD RAZU
        pids_kandydatow[i] = pid;
        baza_wynikow[i].pid = pid;

       if (i % 20 == 0) {
           usleep(100000); 
        }
    }
    loguj(plik_raportu,"[Dziekan] Wszyscy kandydaci (%d) zgromadzeni. Wybija godzina T. Rozpoczynam weryfikacje matur.", liczba_kandydatow);
    //etap 1 weryfikacja matur
    Komunikat msg;

    for (int i = 0; i < liczba_kandydatow; i++){
        //matura od kazdego kandydata
        if (msgrcv(msgid, &msg, sizeof(Komunikat) - sizeof(long), MSG_MATURA_REQ, 0) != -1) {
            int idx = znajdz_lub_dodaj_studenta(msg.nadawca_pid);
            if (idx == -1) {
                loguj(plik_raportu,"BLAD: Nieznany PID kandydata: %d", msg.nadawca_pid);
                continue;
            }
            Komunikat odp;
            odp.mtype = msg.nadawca_pid;
            
            //losowanie matury (2% szans na brak)
            if (losuj(1,100) <= 2) {
                baza_wynikow[idx].matura_zdana = 0;
                odp.dane_int = 0; 
            } else {
                baza_wynikow[idx].matura_zdana = 1;
                odp.dane_int = 1;
            }
            msgsnd(msgid, &odp, sizeof(Komunikat) - sizeof(long), 0);
        }
    }

    //drukowanie list startowych
    drukuj_listy_startowe();

    //etap 2 egzamin wlasciwy
    int zakonczonych_procesow = 0;  
    int czy_koniec = 0;  
    
    while(1){        
        //sprawdzanie czy ktos skonczyl
        int status;
        pid_t zakonczony_pid;

        while ((zakonczony_pid = waitpid(-1, &status, WNOHANG)) > 0) {
            int idx = znajdz_lub_dodaj_studenta(zakonczony_pid);
            if (idx != -1) {
                zakonczonych_procesow++;
                if (baza_wynikow[idx].matura_zdana == 1) {
                    loguj(plik_raportu,"[Dziekan] Student %d (PID: %d) skonczyl. A: %d%%, B: %d%%", 
                        baza_wynikow[idx].id, zakonczony_pid, 
                        baza_wynikow[idx].ocena_koncowa_A, baza_wynikow[idx].ocena_koncowa_B);
                }
                if (zakonczonych_procesow == liczba_kandydatow) {
                    loguj(plik_raportu,"[Dziekan] Wszyscy kandydaci (i odrzuceni) opuscili system.");
                    czy_koniec = 1;
                    break;
                }
            }
        }
        if (czy_koniec){
            break;
        }
        usleep(100000);
    }
    sleep(1);
    generuj_ranking();
    sprzatanie();

    return 0;
}