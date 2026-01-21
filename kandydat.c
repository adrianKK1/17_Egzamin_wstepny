#include "common.h"
#include "utils.h"

int msgid = -1; 
int semid = -1; 



int main() {
    
    pid_t moj_pid = getpid();
    srand(time(NULL) ^ moj_pid); // inicjalizacja generatora liczb losowych

    //losowanie czy kandydat jest poprawkowiczem (2% szans)
    int czy_poprawkowicz = (rand() % 100 < 2) ? 1 : 0;
    printf(ANSI_BROWN "[KANDYDAT %d] Start procesu. Poprawkowicz: %s\n", moj_pid, czy_poprawkowicz ? "TAK" : "NIE" ANSI_RESET "\n");

    //1 podlacznie do IPC, dziekan już stworzył kolejke i semafory
    key_t key = ftok(PROG_SCIEZKA, PROG_ID);
    msgid = msgget(key, 0);//0 = podłączenie do istniejącej kolejki
    semid = semget(key, 4, 0); //0 = podłączenie do istniejącego zbioru semaforów

    if (msgid == -1 || semid == -1) {
        printf(ANSI_RED "[KANDYDAT %d] Blad podlaczenia do IPC - uczelnia zamknieta." ANSI_RESET "\n",moj_pid);
        exit(1);
    }
    
    // 2. Czekanie na start egzaminu (Semafor START)
    // Dziekan podniesie ten semafor o godzinie T. Na razie jest 0.

    // etap 1 - weryfikacja matury
    printf(ANSI_BROWN "[KANDYDAT %d] Idę do Dziekana sprawdzić maturę." ANSI_RESET "\n" , moj_pid);
    operacja_semafor(semid,SEM_DOSTEP_IDX, -1);
    Komunikat msg;
    msg.mtype = MSG_MATURA_REQ;
    msg.nadawca_pid = moj_pid;
    msg.status_specjalny = czy_poprawkowicz;
    msg.dane_int = 0; //brak danych dodatkowych
    

    //wyslanie zapytania do dziekana
    if (msgsnd(msgid, &msg, sizeof(Komunikat) - sizeof(long), 0) == -1) {
        perror(ANSI_RED"[KANDYDAT] Blad wysylania zapytania do Dziekana" ANSI_RESET);
        exit(1);
    }
    //oczekiwanie na odpowiedz od dziekana
    if (msgrcv(msgid, &msg, sizeof(Komunikat) - sizeof(long), moj_pid, 0) == -1) {
        perror(ANSI_RED"[KANDYDAT] Blad odbioru odpowiedzi od Dziekana" ANSI_RESET);
        exit(1);
    }

    operacja_semafor(semid,SEM_DOSTEP_IDX, 1);

    if(msg.dane_int == 0) {
        printf(ANSI_BROWN "[KANDYDAT %d] Matura niezaliczona. Kończę proces." ANSI_RESET "\n", moj_pid);
        exit(0);
    } else {
        printf(ANSI_BROWN "[KANDYDAT %d] Matura zaliczona. Idę na egzaminy." ANSI_RESET "\n", moj_pid);
    }

    // etap 2 - komisja A
    // a) zajmowanie miejsca w semaforze Komisji A (operacja P)
    //blokowanie miejsca w komisji A jesli w sali jest juz 3  studentow
    printf(ANSI_BROWN "[KANDYDAT %d] Czekam na wolne miejsce w Komisji A..." ANSI_RESET "\n", moj_pid);
    operacja_semafor(semid,SEM_KOMISJA_A_IDX, -1); //P(SEM_KOMISJA_A_IDX)
    printf(ANSI_BROWN "[KANDYDAT %d] Zajmuję miejsce w sali Komisji A." ANSI_RESET "\n", moj_pid);

    int czas_Ti = losuj(1, 2); 
    sleep(czas_Ti); //symulacja czasu dotarcia do komisji

    // b) wysyłanie komunikatu do Komisji A
    msg.mtype = MSG_WEJSCIE_A;
    msg.nadawca_pid = moj_pid;
    msg.status_specjalny = czy_poprawkowicz;
    
    if(msgsnd(msgid, &msg, sizeof(Komunikat) - sizeof(long),0) == -1) {        
        perror(ANSI_RED"[KANDYDAT] Blad wyslania komunikatu do Komisji A" ANSI_RESET);
        exit (1);
    }
    // c) oczekiwanie na wynik od Komisji A (blokujaco)
    if (msgrcv(msgid, &msg, sizeof(Komunikat) - sizeof(long), moj_pid, 0) == -1) {
        perror(ANSI_RED"[KANDYDAT] Blad odbioru wyniku od Komisji A" ANSI_RESET);
        exit(1);
    }
    int ocena_a = msg.dane_int;
    printf(ANSI_BROWN "[KANDYDAT %d] Otrzymałem wynik z Komisji A: %d%%" ANSI_RESET "\n", moj_pid, ocena_a);

    // d) zwalnianie miejsca w semaforze Komisji A (operacja V)
    operacja_semafor(semid,SEM_KOMISJA_A_IDX, 1); 
    printf(ANSI_BROWN "[KANDYDAT %d] Zwolniłem miejsce w Komisji A." ANSI_RESET "\n", moj_pid);
    //warunek przejscia do komisji B >30%
    if (ocena_a < 30) {
        printf(ANSI_BROWN "[KANDYDAT %d] Ocena z Komisji A poniżej 30%%. Kończę egzamin." ANSI_RESET "\n", moj_pid);
        exit(0);
    }

    // etap 3 - komisja B
    // a) zajmowanie miejsca w semaforze Komisji B (operacja P)
    printf(ANSI_BROWN "[KANDYDAT %d] Czekam na wolne miejsce w Komisji B..." ANSI_RESET "\n", moj_pid);
    operacja_semafor(semid,SEM_KOMISJA_B_IDX, -1); 
    printf(ANSI_BROWN "[KANDYDAT %d] Zajmuję miejsce w sali Komisji B." ANSI_RESET "\n", moj_pid);
    
    sleep(losuj(1, 2));
    // b) wysyłanie komunikatu do Komisji B
    msg.mtype = MSG_WEJSCIE_B;  
    msg.nadawca_pid = moj_pid;
    msg.status_specjalny = 0;
    if(msgsnd(msgid, &msg, sizeof(Komunikat) - sizeof(long),0) == -1) {        
        perror(ANSI_RED"[KANDYDAT] Blad wyslania komunikatu do Komisji B" ANSI_RESET);
        exit (1);
    }
    // c) oczekiwanie na wynik od Komisji B (blokujaco)
    if (msgrcv(msgid, &msg, sizeof(Komunikat) - sizeof(long), moj_pid, 0) == -1) {
        perror(ANSI_RED"[KANDYDAT] Blad odbioru wyniku od Komisji B" ANSI_RESET);
        exit(1);
    }
    
    int ocena_b = msg.dane_int; 
    printf(ANSI_BROWN "[KANDYDAT %d] Otrzymałem wynik z Komisji B: %d%%" ANSI_RESET "\n", moj_pid, ocena_b);

    // d) zwalnianie miejsca w semaforze Komisji B (operacja V)
    operacja_semafor(semid,SEM_KOMISJA_B_IDX, 1);
    printf(ANSI_BROWN "[KANDYDAT %d] Zwolniłem miejsce w Komisji B. Koniec egzaminu." ANSI_RESET "\n", moj_pid);
    return 0;
}