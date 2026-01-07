#include "common.h"
#include "utils.h"
#include <pthread.h>

//zmienne globalne dla wątków
int msgid = -1;
char typ_komisji; // 'A' lub 'B'
int liczba_egzaminatorow; 

//struktura kolejaki wewnętrznej dla wątków (watek glowny -> watki robocze)
//to jest bufor w ktorym wątek główny będzie przekazywał zadania do wątków roboczych (recepcja zostawia studentow dla egzaminatorow)
typedef struct Wezel {
    Komunikat dane_studenta;
    struct Wezel* next;
} Wezel;

Wezel* head = NULL; // wskaznik na poczatek kolejki
Wezel* tail = NULL; // wskaznik na koniec kolejki

//synchronizacja wątków
pthread_mutex_t mutex_kolejki = PTHREAD_MUTEX_INITIALIZER; //muteks do ochrony kolejki
pthread_cond_t cond_kolejki = PTHREAD_COND_INITIALIZER;   //warunek do sygnalizowania obecności nowych zadań w kolejce

//funkcja dodająca studenta do kolejki wewnętrznej
void dodaj_do_kolejki_wew(Komunikat msg){
    Wezel* nowy = malloc(sizeof(Wezel));
    if(!nowy){ perror("Blad alokacji pamieci dla wezla kolejki wewnetrznej"); exit(1);}
    nowy->dane_studenta = msg;
    nowy->next = NULL;

    //sekcja krytyczna - ochrona muteksem 
    pthread_mutex_lock(&mutex_kolejki);
    if(tail == NULL){ //kolejka pusta
        head = tail = nowy;
    } else {
        tail->next = nowy;
        tail = nowy;
    }

    //sygnalizacja ze jest nowy student w kolejce - budzimy jednego z oczekujacych egzaminatorow
    pthread_cond_signal(&cond_kolejki);
    pthread_mutex_unlock(&mutex_kolejki);
}

//funkcja pobierająca z kolejki wewnętrznej studenta do obsługi przez egzaminatora
Komunikat pobierz_z_kolejki_wew(){
    pthread_mutex_lock(&mutex_kolejki);
    while(head == NULL){ //kolejka pusta, czekamy na sygnal
        pthread_cond_wait(&cond_kolejki, &mutex_kolejki);
    }
    Wezel* temp = head;
    Komunikat msg = temp->dane_studenta;
    head = head->next;
    if(head == NULL) tail = NULL; //kolejka pusta po pobraniu
    free(temp);
    pthread_mutex_unlock(&mutex_kolejki);
    return msg;
}

//LOGIKA EGZAMINATORA
void* watek_egzaminatora(void* arg) {
    int id_egzaminatora = *((int*)arg);
    free(arg); //zwalniamy pamiec po id egzaminatora
    printf("[Komisja %c] Egzaminator %d gotowy do pracy .\n", typ_komisji, id_egzaminatora);

    while(1) {
        //1. Czekaj na studenta - Pobierz studenta z kolejki wewnętrznej
        Komunikat student = pobierz_z_kolejki_wew();
        printf("[Komisja %c] Egzaminator %d rozpoczyna egzamin studenta PID: %d.\n", typ_komisji, id_egzaminatora, student.nadawca_pid);

        //2. Przeprowadz egzamin (symulacja czasem snu)
        int czas_egzaminu = (rand() % 3) + 2; //czas egzaminu 2-4 sekundy
        sleep(czas_egzaminu);

        //3. Wystawianie oceny (0-100%)
        int ocena = losuj(0, 100);

        //4. Wsylanie wyniku do dziekana (do rankingu)
        Komunikat wynik_dla_dziekana;
        wynik_dla_dziekana.mtype = MSG_WYNIKI; //typ komunikatu dla dziekana
        wynik_dla_dziekana.nadawca_pid = student.nadawca_pid; //ID studenta
        wynik_dla_dziekana.dane_int = ocena; //ocena

        // Zakodowanie w treści, od której to komisji (np. "A:55")
        snprintf(wynik_dla_dziekana.tresc, 20, "%c:%d", typ_komisji, ocena);

        if (msgsnd(msgid, &wynik_dla_dziekana, sizeof(Komunikat) - sizeof(long), 0) == -1) {
            perror("Bład wysylania wyniku do Dziekana");
        }

        //wyslanie wyniku do studenta - aby mogl isc dalej
        //adresujemy po PID studenta
        Komunikat wynik_dla_studenta;
        wynik_dla_studenta.mtype = student.nadawca_pid; //typ
        wynik_dla_studenta.dane_int = ocena; //ocena
        snprintf(wynik_dla_studenta.tresc, 256, "Komisja %c zakonczona. Ocena: %d%%", typ_komisji, ocena);

        if (msgsnd(msgid, &wynik_dla_studenta, sizeof(Komunikat) - sizeof(long), 0) == -1) {
            perror("Blad wysylania wyniku do studenta");
        }

        printf("[KOMISJA %c] Egzaminator %d: Zakończono egzamin PID=%d, Ocena=%d%%\n", 
               typ_komisji, id_egzaminatora, student.nadawca_pid, ocena);
    }
    return NULL;
}

int main(int argc, char* argv[]) {
    //sprawdzenie argumentow
    if (argc != 2){ 
        fprintf(stderr, "Uzycie: %s <typ_komisji: A|B>\n", argv[0]);
        exit(1);
    }
    //ustawienie typu komisji i liczby egzaminatorów
    typ_komisji = argv[1][0];
    if (typ_komisji == 'A'){
        liczba_egzaminatorow = 5;
    } else if (typ_komisji == 'B'){
        liczba_egzaminatorow = 3;
    } else {
        fprintf(stderr, "Nieprawidlowy typ komisji. Uzyj 'A' lub 'B'.\n");
        exit(1);
    }
    //1 podlacznie do IPC, dziekan już stworzył kolejke
    key_t key = ftok(PROG_SCIEZKA, PROG_ID);
    msgid = msgget(key, 0);//0 = podłączenie do istniejącej kolejki
    check_error(msgid, "[KOMISJA] blad msgget - czy dziekan jest uruchomiony?");
    
    printf("[KOMISJA %c] Start systemu. Uruchamiam %d watkow egzaminatorow.\n", typ_komisji, liczba_egzaminatorow);

    //2. tworzenie puli watkow 
    pthread_t* watki=malloc(sizeof(pthread_t) * liczba_egzaminatorow);
    for (int i=0; i < liczba_egzaminatorow; i++) {
        int* id = malloc(sizeof(int));
        *id = i + 1; //id egzaminatora od 1
        if (pthread_create(&watki[i], NULL, watek_egzaminatora, id) != 0) {
            perror("Blad tworzenia watku egzaminatora");
            exit(1);
        }
    }

    //3 petla glowan (recepcja) - odbieranie studentow z kolejki i przekazywanie do egzaminatorow
    long typ_odbioru = (typ_komisji == 'A') ? MSG_WEJSCIE_A : MSG_WEJSCIE_B;
    Komunikat msg;

    while(1) {
        //msgrcv z flaga 0 - blokujace oczekiwanie na studenta
        if (msgrcv(msgid, &msg, sizeof(Komunikat) - sizeof(long), typ_odbioru, 0) != -1) {
            printf("[KOMISJA %c] Otrzymano studenta PID=%d. Przekazuje do kolejki.\n", typ_komisji, msg.nadawca_pid);

            //przekazanie do watkow 
            dodaj_do_kolejki_wew(msg);
        } else {
            //blad odbioru msgrcv (np, dziekan usunal kolejke - koniec symulacji)
            if (errno == EIDRM || errno == EINVAL) {
                printf("[KOMISJA %c] Kolejka komunikatow zostala usunieta. Koncze dzialanie.\n", typ_komisji);
                break;
            } else {
                perror("[KOMISJA] Blad odbioru msgrcv");
            }
        }
    }

    //sprzatanie pamieci
    free(watki);

    return 0;
}