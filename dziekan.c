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

//tablica wynikow
StudentWynik *baza_wynikow = NULL;

//definicja unii dla semctl
union semun {
    int val;                // wartosc dla SETVAL
    struct semid_ds *buf;   // bufor dla IPC_STAT, IPC_SET
    unsigned short *array;  // tablica dla GETALL, SETALL
};

//funkcja zapisujaca logi do pliku i na ekran
void loguj(const char *format, ...) {
    va_list args;
    char bufor[1024];
    char czas[30]; 

    //pobiernaie czasu
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(czas, sizeof(czas), "%H:%M:%S", t);

    //formatownanie komunikatu
    va_start(args, format);
    vsnprintf(bufor, sizeof(bufor), format, args);
    va_end(args);

    //wypisanie na ekran
    printf("[%s] %s\n", czas, bufor);

    //zapis do pliku
    if (plik_raportu) {
        fprintf(plik_raportu, "[%s] %s\n", czas, bufor);
        fflush(plik_raportu); //natychmiastowy zapis
    }
}

//funkcja szukajaca studenta w bazie po pid
//jesli nie ma dodaje go w pierwsze wolne miejsce lub zwaraca istniejacy indeks
int znajdz_lub_dodaj_studenta(pid_t pid) {
    for (int i = 0; i < liczba_kandydatow; i++) {
        if (baza_wynikow[i].pid == pid) {
            return i; //znaleziono
        }
    }
    //nie znaleziono, dodajemy nowego 
    for (int i = 0; i < liczba_kandydatow; i++) {
        if (baza_wynikow[i].pid == 0) {
            baza_wynikow[i].pid = pid;
            baza_wynikow[i].id = i + 1;
            baza_wynikow[i].matura_zdana = -1;
            baza_wynikow[i].ocena_A = -1;
            baza_wynikow[i].ocena_B = -1;
            baza_wynikow[i].suma_ocen = 0;
            return i;
        }
    }
    return -1; //brak miejsca
}

void drukuj_listy_startowe() {
    loguj("\n[Dziekan] ZAKONCZONO WERYFIKACJE MATUR. PUBLIKACJA LIST STARTOWYCH.");

    //1 lista niedopuszczonych
    if (plik_raportu) {
        fprintf(plik_raportu, "\n===LISTA NIEDOPUSZCZONYCH (BRAK MATURY) ===\n");
    }
    printf("\n--- LISTA OSOB NIEDOPUSZCZONYCH (BRAK MATURY) ---\n");

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
            printf("BRAK OSOB NIEDOPUSZCZONYCH\n");
            if (plik_raportu) {
                fprintf(plik_raportu, "BRAK OSOB NIEDOPUSZCZONYCH\n");
            }
        }
    //2 lista dopuszczonych
    if (plik_raportu) {
        fprintf(plik_raportu, "\n===LISTA DOPUSZCZONYCH DO EGZAMINU ===\n");
    }
    printf("\n--- LISTA OSOB DOPUSZCZONYCH DO EGZAMINU ---\n");

    int dopuszczeni_count = 0;
        for (int i = 0; i < liczba_kandydatow; i++) {
            if(baza_wynikow[i].matura_zdana == 1) {
                //snprintf(bufor, sizeof(bufor), "Student nr %d (PID: %d) - DOPUSZCZONY", baza_wynikow[i].id, baza_wynikow[i].pid);
               // printf("%s\n", bufor);
                if (plik_raportu) {
                    fprintf(plik_raportu, "Student nr %d (PID: %d) - DOPUSZCZONY", baza_wynikow[i].id, baza_wynikow[i].pid);
                    dopuszczeni_count++;
                }
            }
        }
        printf("Statystyki wstepne: Dopuszczeni: %d, Odrzuceni: %d.", dopuszczeni_count, brak_matury_count);
        loguj("[Dziekan] Statystyki wstepne: Miejsc: %d, Kandydatow: %d, Dopuszczeni: %d. Start egzaminow.", liczba_miejsc, liczba_kandydatow, dopuszczeni_count);
        if (plik_raportu) fprintf(plik_raportu, "--------------------------------------------------\n\n");

}

// funkcja sortujaca wyniki studentow wedlug sumy ocen malejaco
int porownaj_studentow(const void *a, const void *b) {
    StudentWynik *s1 = (StudentWynik *)a;
    StudentWynik *s2 = (StudentWynik *)b;

    int s1_zaliczyl = (s1->pid != 0 && s1->matura_zdana == 1 && s1->ocena_A >= 30 && s1->ocena_B >= 30);
    int s2_zaliczyl = (s2->pid != 0 && s2->matura_zdana == 1 && s2->ocena_A >= 30 && s2->ocena_B >= 30);

    if (s1_zaliczyl && !s2_zaliczyl) return -1;
    if (!s1_zaliczyl && s2_zaliczyl) return 1;
    if (s1_zaliczyl && s2_zaliczyl) {
        return s2->suma_ocen - s1->suma_ocen; 
    }
    return s1->id - s2->id;
    
}

//funkcja tworzaca koncowy rangking
void generuj_ranking() {
    loguj("\n [Dziekan] KONIEC EGZAMINU. OBLICZANIE RANKINGU I PUBLIKACJA WYNIKOW");

    //obliczanie sumyp unktow dla kazdego
    for (int i = 0; i < liczba_kandydatow; i++) {
        if (baza_wynikow[i].pid != 0) {
           int pkt_A = (baza_wynikow[i].ocena_A == -1) ? 0 : baza_wynikow[i].ocena_A;
           int pkt_B = (baza_wynikow[i].ocena_B == -1) ? 0 : baza_wynikow[i].ocena_B;
           baza_wynikow[i].suma_ocen = pkt_A + pkt_B;
        }
    }
    //sortowanie
    qsort(baza_wynikow, liczba_kandydatow, sizeof(StudentWynik), porownaj_studentow);

    if (plik_raportu) {
        fprintf(plik_raportu, "\n=================================================================================\n");
        fprintf(plik_raportu, "| %-4s | %-10s | %-8s | %-8s | %-8s | %-5s | %-24s |\n", "NR", "PID", "MATURA", "KOM A", "KOM B", "SUMA", "STATUS");
        fprintf(plik_raportu, "=================================================================================\n");
    }

    printf("\n==== LISTA RANKINGOWA (Limit miejsc: %d) =====\n", liczba_miejsc);
    printf("| %-4s | %-10s | %-8s | %-8s | %-8s | %-5s | %-24s |\n", "NR", "PID", "MATURA", "OCENA A", "OCENA B", "SUMA", "STATUS");
    
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
        if (s.ocena_A == -1){
            strcpy(s_ocenaA, "---");
        } else {
            sprintf(s_ocenaA, "%d%%", s.ocena_A);
        }

        // Opis Oceny B
        if (s.ocena_B == -1){
            strcpy(s_ocenaB, "---");
        } else {
            sprintf(s_ocenaB, "%d%%", s.ocena_B);
        }

        //status końcowy
        int czy_zdal = (s.matura_zdana == 1 && s.ocena_A >= 30 && s.ocena_B >= 30);
        
        if (s.matura_zdana == 0) {
            // 1 Odrzucony przez brak matury
            strcpy(status, "ODRZUCONY - BRAK MATURY");
        } 
        else if (s.ocena_A != -1 && s.ocena_A < 30) { 
            // 2 Oblal egzamin A (ma ocene i jest < 30)
            strcpy(status, "NIEZDAL (KOMISJA A)");
        } 
        else if (s.ocena_B != -1 && s.ocena_B < 30) {
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

        // Wypisanie
        printf("| %-4d | %-10d | %-8s | %-8s | %-8s | %-5d | %-24s |\n", s.id, s.pid, s_matura, s_ocenaA, s_ocenaB, s.suma_ocen, status);
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
    printf("\n\n====  LISTA PRZYJETYCH  ====\n");
    printf("| %-4s | %-10s | %-8s | %-8s | %-12s |\n", "NR", "PID", "OCENA A", "OCENA B", "OCENA KONCOWA");

    przyjetych_count = 0; 
    for (int i = 0; i < liczba_kandydatow; i++) {
        if (baza_wynikow[i].pid == 0) continue;
        
        StudentWynik s = baza_wynikow[i];
        
        // Warunek zaliczenia
        int czy_zdal = (s.matura_zdana == 1 && s.ocena_A >= 30 && s.ocena_B >= 30);
        
        if (czy_zdal) {
            if (przyjetych_count < liczba_miejsc) {
                // To jest osoba przyjeta - wypisujemy ja na drugiej liscie
                char s_ocenaA[10], s_ocenaB[10];
                sprintf(s_ocenaA, "%d%%", s.ocena_A);
                sprintf(s_ocenaB, "%d%%", s.ocena_B);

                printf("| %-4d | %-10d | %-8s | %-8s | %-13d |\n", s.id, s.pid, s_ocenaA, s_ocenaB, s.suma_ocen);
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
    loguj("[Dziekan] Zamykanie systemu i sprzatanie zasobow IPC...");

    //zabijanie komisji
    if (pid_komisja_A > 0) {
        kill(pid_komisja_A, SIGTERM);
    }
    if (pid_komisja_B > 0) {
        kill(pid_komisja_B, SIGTERM);
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
    printf("\n[Dziekan] Otrzymano sygnal EWAKUACJA. Generowanie rankingu oraz zamkniecie uczelni...\n");
    generuj_ranking();
    sprzatanie();
    exit(0);
}


int main(int argc, char *argv[]) {
    // Deklaracja zmiennej pomocniczej 
    pid_t pid; 

    //domyslna liczba kandydatow
    int M_miejsc = (argc >1) ? atoi(argv[1]) : 10; 
    int ilosc_chetnych = M_miejsc * 10;

    liczba_kandydatow = ilosc_chetnych;
    liczba_miejsc = M_miejsc;

    //raport otwarcie pliku
    plik_raportu = fopen("raport.txt", "w");
    if (!plik_raportu) {
        perror("Nie mozna otworzyc pliku raportu");
        exit(1);
    }

    signal(SIGINT, obsluga_sigint); //obsluga ctrl+c

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
        baza_wynikow[i].ocena_A = -1;
        baza_wynikow[i].ocena_B = -1;
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
    semctl(semid, SEM_START_IDX, SETVAL, arg);//semafor startu na 0
    arg.val = 50; // Wpuszczamy max 50 osob naraz do kolejki matur
    semctl(semid, SEM_DOSTEP_IDX, SETVAL, arg);
    
   loguj("[Dziekan] Uczelnia otwarta. Miejsc: %d. Przewidywana liczba kandydatow: %d.", liczba_miejsc, liczba_kandydatow);
   
    char buf_liczba[20];
    sprintf(buf_liczba, "%d", liczba_kandydatow);
    //uruchomienie komisji
    //komisja A
    if ((pid_komisja_A = fork()) == 0) {
        execl("./komisja", "komisja", "A", buf_liczba, NULL); 
        perror("Blad uruchamiania komisji A"); 
        exit(1);
    }
    if ((pid_komisja_B = fork()) == 0) {
        execl("./komisja", "komisja", "B", buf_liczba, NULL); 
        perror("Blad uruchamiania komisji B"); 
        exit(1);
    }

    loguj("[Dziekan] Komisja A (PID: %d) i komisja B (PID: %d) rozpoczynaja prace.", pid_komisja_A, pid_komisja_B);
    //sleep(1); //chwila na uruchomienie komisji

    //3 uruchomienie kandydatow
    pids_kandydatow = malloc(sizeof(pid_t) * liczba_kandydatow);
    for (int i = 0; i < liczba_kandydatow; i++) {
        pid = fork(); 
        if (pid == 0) {
            execl("./kandydat", "kandydat", NULL);
            perror("Blad uruchamiania kandydata");
            exit(1);
        }
        
        // Zapisanie danych kandydata w bazie OD RAZU
        pids_kandydatow[i] = pid;
        baza_wynikow[i].pid = pid;

        //if (i % 20 == 0) {
            //usleep(100000); 
       // }
    }
    loguj("[Dziekan] Wszyscy kandydaci (%d) zgromadzeni. Wybija godzina T. Rozpoczynam weryfikacje matur.", liczba_kandydatow);
    //etap 1 weryfikacja matur
    Komunikat msg;

    for (int i = 0; i < liczba_kandydatow; i++){
        //matura od kazdego kandydata
        if (msgrcv(msgid, &msg, sizeof(Komunikat) - sizeof(long), MSG_MATURA_REQ, 0) != -1) {
            int idx = znajdz_lub_dodaj_studenta(msg.nadawca_pid);
            if (idx == -1) {
                loguj("BLAD: Nieznany PID kandydata: %d", msg.nadawca_pid);
                continue;
            }
            Komunikat odp;
            odp.mtype = msg.nadawca_pid;
            
            //losowanie matury (2% szans na brak)
            if (losuj(1,100) <= 2) {
                baza_wynikow[idx].matura_zdana = 0;
                odp.dane_int = 0; //niezdal
                //loguj("[Weryfikacja] Kandydat nr %d (PID: %d) -> BRAK MATURY. Odrzucony.", baza_wynikow[idx].id, msg.nadawca_pid);
            } else {
                baza_wynikow[idx].matura_zdana = 1;
                odp.dane_int = 1; //zdal
                //loguj("[Weryfikacja] Kandydat nr %d (PID: %d) -> MATURA ZALICZONA. Dopuszczony do egzaminu.", baza_wynikow[idx].id, msg.nadawca_pid);
            }
            msgsnd(msgid, &odp, sizeof(Komunikat) - sizeof(long), 0);
        }
    }

    //drukowanie list startowych
    drukuj_listy_startowe();

    //etap 2 egzamin wlasciwy
    //glowna petla obslugi
    int zakonczonych_procesow = 0;    
    
    while(1){        
        
        //sprawdzanie czy ktos skonczyl
        int status;
        pid_t zakonczony_pid = waitpid(-1, &status, WNOHANG);
        if (zakonczony_pid > 0) {
            // Czy to byl kandydat?
            int idx = znajdz_lub_dodaj_studenta(zakonczony_pid);
            if (idx != -1) {
                zakonczonych_procesow++;

                // odczytanie wynikow z pamieci dzielonej
                if (baza_wynikow[idx].matura_zdana == 1) {
                    loguj("[Dziekan] Student %d (PID: %d) skonczyl. A: %d%%, B: %d%%", 
                        baza_wynikow[idx].id, zakonczony_pid, 
                        baza_wynikow[idx].ocena_A, baza_wynikow[idx].ocena_B);
                }
                
                /* Mozemy sprawdzic dlaczego skonczyl
                if (baza_wynikow[idx].matura_zdana == 0) {
                    loguj("[Dziekan] Kandydat nr %d (PID: %d) zakonczyl proces - ODRZUCONY (BRAK MATURY).", baza_wynikow[idx].id, zakonczony_pid);
                } else {
                    loguj("[Dziekan] Kandydat nr %d (PID: %d) zakonczyl egzaminy.", baza_wynikow[idx].id, zakonczony_pid);
                }
                */
                if (zakonczonych_procesow == liczba_kandydatow) {
                    loguj("[Dziekan] Wszyscy kandydaci (i odrzuceni) opuscili system.");
                    break;
                }
            }
        }
        //usleep(50000); //chwila przerwy
    }

    //koniec symulacji
   //sleep(1);
    generuj_ranking();
    sprzatanie();

    return 0;
}