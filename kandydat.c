#include "common.h"
#include "utils.h"

int msgid = -1; 
int semid = -1; 

//funkcje pomocnicze do operacji na semgaforach (P - czrekaj, V - sygnalizuj)
void operacja_semafor(int sem_idx, int op){
    struct sembuf bufor; 
    bufor.sem_num = sem_idx; //indeks semafora w zbiorze
    bufor.sem_op = op; //-1 czekaj/zajmiij, lub 1 zwolnij
    bufor.sem_flg = 0; //brak flag specjalnych

    //jesli semafor jest zajety, proces zostanie zablokowany tutaj
    if (semop(semid, &bufor, 1) == -1) {
        //igonorujemy blad przerwania przez dziekana
        if (errno != EINTR && errno != EIDRM) {
            perror("[KOMISJA] Blad operacji na semaforze");
            exit(1);
        } else {
            exit(0); 
        }
    }
}

int main() {
    
    pid_t moj_pid = getpid();
    srand(time(NULL) ^ moj_pid); // inicjalizacja generatora liczb losowych

    //losowanie czy kandydat jest poprawkowiczem (2% szans)
    int czy_poprawkowicz = (rand() % 100 < 2) ? 1 : 0;
    printf("[KANDYDAT %d] Start procesu. Poprawkowicz: %s\n", moj_pid, czy_poprawkowicz ? "TAK" : "NIE");

    //1 podlacznie do IPC, dziekan już stworzył kolejke i semafory
    key_t key = ftok(PROG_SCIEZKA, PROG_ID);
    msgid = msgget(key, 0);//0 = podłączenie do istniejącej kolejki
    semid = semget(key, 4, 0); //0 = podłączenie do istniejącego zbioru semaforów

    if (msgid == -1 || semid == -1) {
        printf("[KANDYDAT %d] Blad podlaczenia do IPC - uczelnia zamknieta.\n",moj_pid);
        exit(1);
    }
    
    // 2. Czekanie na start egzaminu (Semafor START)
    // Dziekan podniesie ten semafor o godzinie T. Na razie jest 0.

    // etap 1 - weryfikacja matury
    printf("[KANDYDAT %d] Idę do Dziekana sprawdzić maturę.\n", moj_pid);
    operacja_semafor(SEM_DOSTEP_IDX, -1);
    Komunikat msg;
    msg.mtype = MSG_MATURA_REQ;
    msg.nadawca_pid = moj_pid;
    msg.status_specjalny = czy_poprawkowicz;
    msg.dane_int = 0; //brak danych dodatkowych
    

    //wyslanie zapytania do dziekana
    if (msgsnd(msgid, &msg, sizeof(Komunikat) - sizeof(long), 0) == -1) {
        perror("[KANDYDAT] Blad wysylania zapytania do Dziekana");
        exit(1);
    }
    //oczekiwanie na odpowiedz od dziekana
    if (msgrcv(msgid, &msg, sizeof(Komunikat) - sizeof(long), moj_pid, 0) == -1) {
        perror("[KANDYDAT] Blad odbioru odpowiedzi od Dziekana");
        exit(1);
    }

    operacja_semafor(SEM_DOSTEP_IDX, 1);

    if(msg.dane_int == 0) {
        printf("[KANDYDAT %d] Matura niezaliczona. Kończę proces.\n", moj_pid);
        exit(0);
    } else {
        printf("[KANDYDAT %d] Matura zaliczona. Idę na egzaminy.\n", moj_pid);
    }

    // etap 2 - komisja A
    // a) zajmowanie miejsca w semaforze Komisji A (operacja P)
    //blokowanie miejsca w komisji A jesli w sali jest juz 3  studentow
    printf("[KANDYDAT %d] Czekam na wolne miejsce w Komisji A...\n", moj_pid);
    operacja_semafor(SEM_KOMISJA_A_IDX, -1); //P(SEM_KOMISJA_A_IDX)
    printf("[KANDYDAT %d] Zajmuję miejsce w sali Komisji A.\n", moj_pid);

    //int czas_Ti = losuj(1, 2); 
    //sleep(czas_Ti); //symulacja czasu dotarcia do komisji

    // b) wysyłanie komunikatu do Komisji A
    msg.mtype = MSG_WEJSCIE_A;
    msg.nadawca_pid = moj_pid;
    msg.status_specjalny = czy_poprawkowicz;
    
    if(msgsnd(msgid, &msg, sizeof(Komunikat) - sizeof(long),0) == -1) {        
        perror("[KANDYDAT] Blad wyslania komunikatu do Komisji A");
        exit (1);
    }
    // c) oczekiwanie na wynik od Komisji A (blokujaco)
    if (msgrcv(msgid, &msg, sizeof(Komunikat) - sizeof(long), moj_pid, 0) == -1) {
        perror("[KANDYDAT] Blad odbioru wyniku od Komisji A");
        exit(1);
    }
    int ocena_a = msg.dane_int;
    printf("[KANDYDAT %d] Otrzymałem wynik z Komisji A: %d%%\n", moj_pid, ocena_a);

    // d) zwalnianie miejsca w semaforze Komisji A (operacja V)
    operacja_semafor(SEM_KOMISJA_A_IDX, 1); 
    printf("[KANDYDAT %d] Zwolniłem miejsce w Komisji A.\n", moj_pid);

    //warunek przejscia do komisji B >30%
    if (ocena_a < 30) {
        printf("[KANDYDAT %d] Ocena z Komisji A poniżej 30%%. Kończę egzamin.\n", moj_pid);
        exit(0);
    }

    // etap 3 - komisja B
    // a) zajmowanie miejsca w semaforze Komisji B (operacja P)
    printf("[KANDYDAT %d] Czekam na wolne miejsce w Komisji B...\n", moj_pid);
    operacja_semafor(SEM_KOMISJA_B_IDX, -1); 
    printf("[KANDYDAT %d] Zajmuję miejsce w sali Komisji B.\n", moj_pid);
    
    //sleep(losuj(1, 2));
    // b) wysyłanie komunikatu do Komisji B
    msg.mtype = MSG_WEJSCIE_B;  
    msg.nadawca_pid = moj_pid;
    msg.status_specjalny = 0;
    if(msgsnd(msgid, &msg, sizeof(Komunikat) - sizeof(long),0) == -1) {        
        perror("[KANDYDAT] Blad wyslania komunikatu do Komisji B");
        exit (1);
    }
    // c) oczekiwanie na wynik od Komisji B (blokujaco)
    if (msgrcv(msgid, &msg, sizeof(Komunikat) - sizeof(long), moj_pid, 0) == -1) {
        perror("[KANDYDAT] Blad odbioru wyniku od Komisji B");
        exit(1);
    }
    
    int ocena_b = msg.dane_int; 
    printf("[KANDYDAT %d] Otrzymałem wynik z Komisji B: %d%%\n", moj_pid, ocena_b);

    // d) zwalnianie miejsca w semaforze Komisji B (operacja V)
    operacja_semafor(SEM_KOMISJA_B_IDX, 1);
    printf("[KANDYDAT %d] Zwolniłem miejsce w Komisji B. Koniec egzaminu.\n", moj_pid);

    return 0;
}