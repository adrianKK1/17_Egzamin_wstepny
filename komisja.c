#include "common.h"
#include "utils.h"
#include <pthread.h>

//zmienne globalne dla wątków
int msgid = -1;
int shmid = -1;
char typ_komisji; // 'A' lub 'B'
int liczba_egzaminatorow; 
int max_studentow = 0;

StudentWynik *baza_shm = NULL;

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
    pthread_mutex_unlock(&mutex_kolejki);
    free(temp);
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
        
        //egzamin
        int ocena_koncowa = 0;
        if (typ_komisji == 'A' && student.status_specjalny == 1) {
            ocena_koncowa = 100;
        } else {
            int suma_ocen = 0;
            for (int i = 0; i < liczba_egzaminatorow; i++) {
                //sleep(losuj(1, 2)); 
                suma_ocen += losuj(0, 100);
            }
            ocena_koncowa = suma_ocen / liczba_egzaminatorow;
        }
        

        //4. Wsylanie wyniku do dziekana (do rankingu)
        int znaleziono = 0;
        for(int i=0; i<max_studentow; i++) {
            if (baza_shm[i].pid == student.nadawca_pid) {
                if (typ_komisji == 'A') baza_shm[i].ocena_A = ocena_koncowa;
                else baza_shm[i].ocena_B = ocena_koncowa;
                znaleziono = 1;
                break;
            }
        }
        if (!znaleziono) {
            printf("[Komisja %c] BLAD: Nie znaleziono studenta PID %d w SHM!\n", typ_komisji, student.nadawca_pid);
        }
        
        
        //wyslanie wyniku do studenta - aby mogl isc dalej
        
        Komunikat wynik_dla_studenta;
        wynik_dla_studenta.mtype = student.nadawca_pid; //typ
        wynik_dla_studenta.dane_int = ocena_koncowa; //ocena
        //snprintf(wynik_dla_studenta.tresc, sizeof(wynik_dla_studenta.tresc), "Komisja %c zakonczona. Ocena: %d%%", typ_komisji, ocena_koncowa);

        if (msgsnd(msgid, &wynik_dla_studenta, sizeof(Komunikat) - sizeof(long), 0) == -1) {
            perror("Blad wysylania wyniku do studenta");
        }

        printf("[KOMISJA %c] Egzaminator %d: Zakończono egzamin PID=%d, Ocena=%d%%\n", 
               typ_komisji, id_egzaminatora, student.nadawca_pid, ocena_koncowa);

        
    }
    return NULL;
}

int main(int argc, char* argv[]) {
    //sprawdzenie argumentow
    if (argc != 3){ 
        fprintf(stderr, "Uzycie: %s <typ: A|B> <liczba_kandydatow>\n", argv[0]);
        exit(1);
    }
    //ustawienie typu komisji i liczby egzaminatorów
    typ_komisji = argv[1][0];
    max_studentow = atoi(argv[2]);
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

    shmid = shmget(key, sizeof(StudentWynik) * max_studentow, 0); // 0 = istniejaca
    check_error(shmid, "[KOMISJA] Blad shmget");
    
    baza_shm = (StudentWynik*)shmat(shmid, NULL, 0);
    if (baza_shm == (void*)-1) { perror("Blad shmat"); exit(1); }

    
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


    return 0;
}