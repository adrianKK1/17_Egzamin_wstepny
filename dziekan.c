#include "common.h"
#include "utils.h"
#include <signal.h>
#include <time.h>
#include <stdarg.h>

//zmienne globalne
int msgid = -1;
int semid = -1;
pid_t pid_komisja_A = -1;
pid_t pid_komisja_B = -1;
pid_t *pids_kandydatow = NULL;
int liczba_kandydatow = 0;
FILE *plik_raportu = NULL;


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
    printf("\n[Dziekan] Otrzymano sygnal SIGINT. Zamykanie procesu...\n");
    sprzatanie();
    exit(0);
}


int main(int argc, char *argv[]) {
    //domyslna liczba kandydatow
    int M = (argc >1) ? atoi(argv[1]) : 10; 
    liczba_kandydatow = M;
    
    //raport otwarcie pliku
    plik_raportu = fopen("raport.txt", "w");
    if (!plik_raportu) {
        perror("Nie mozna otworzyc pliku raportu");
        exit(1);
    }

    signal(SIGINT, obsluga_sigint); //ustawienie handlera SIGINT

    //1 inicjalizacja IPC
    key_t key = ftok(PROG_SCIEZKA, PROG_ID);
    msgid = msgget(key, IPC_CREAT | 0600);
    semid = semget(key, 3, IPC_CREAT | 0600);

    if (msgid == -1 || semid == -1) {
        perror("Blad inicjalizacji IPC");
        exit(1);
    }

    //2 inicjalizacja semaforow
    union semun arg;
    arg.val = LIMIT_SALA;
    semctl(semid, SEM_KOMISJA_A_IDX, SETVAL, arg);//Limit dla komisji A
    semctl(semid, SEM_KOMISJA_B_IDX, SETVAL, arg);//Limit dla komisji B

    arg.val = 0;
    semctl(semid, SEM_START_IDX, SETVAL, arg);//semafor startu na 0
    
    loguj("[Dziekan] Uczelnia otwarta. Liczba miejsc: %d. Kandydatow: %d.", M_MIEJSC, M);

    //uruchomienie komisji
    //komisja A
    if ((pid_komisja_A = fork()) == 0) {
        execl("./komisja", "komisja", "A", NULL);
        perror("Blad uruchamiania komisji A");
        exit(1);
    }
    //komisja B
    if ((pid_komisja_B = fork()) == 0) {
        execl("./komisja", "komisja", "B", NULL);
        perror("Blad uruchamiania komisji B");
        exit(1);
    }

    loguj("[Dziekan] Komisja A (PID: %d) i komisja B (PID: %d) rozpoczynaja prace.", pid_komisja_A, pid_komisja_B);
    sleep(1); //chwila na uruchomienie komisji

    //3 uruchomienie kandydatow
    pids_kandydatow = malloc(sizeof(pid_t) * M);
    for (int i = 0; i < M; i++) {
        if ((pids_kandydatow[i] = fork()) == 0) {
            execl("./kandydat", "kandydat", NULL);
            perror("Blad uruchamiania kandydata");
            exit(1);
        }
        if (i % 5 == 0) {
            usleep(100000); //chwila przerwy co 5 kandydatow
        }
    }
    loguj("[Dziekan] Wpuszczono %d kandydatow. Czekam na wyniki.", M);

    //glowna petla obslugi

    Komunikat msg;
    
    
    while(1){
        //zapytaniao mature
        if (msgrcv(msgid, &msg, sizeof(Komunikat) - sizeof(long), MSG_MATURA_REQ, IPC_NOWAIT) != -1) {
            Komunikat odp;
            odp.mtype = msg.nadawca_pid; //odpowiedz do kandydata

            //2%szans na brak matury
            if (losuj(1,100) <= 2) {
                odp.dane_int = 0;
                loguj("[Dziekan] Kandydat (PID: %d) nie posiada matury. Odrzucenie.", msg.nadawca_pid);
            } else {
                odp.dane_int = 1;
                loguj("[Dziekan] Kandydat (PID: %d) posiada matura. Akceptacja.", msg.nadawca_pid);
            }
            msgsnd(msgid, &odp, sizeof(Komunikat) - sizeof(long), 0);
        }
        
        //obsulga wynikow z komisji 
        if (msgrcv(msgid, &msg, sizeof(Komunikat) - sizeof(long), MSG_WYNIKI, IPC_NOWAIT) != -1) {
            loguj("WYNIK: Student PID=%d -> %s", msg.nadawca_pid, msg.tresc);
        }

        usleep(100000); //chwila przerwy
    }

    return 0;
}