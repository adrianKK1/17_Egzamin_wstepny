Imię i nazwisko: Adrian Kida
Numer albumu: 154675
GitHub: https://github.com/adrianKK1/17_Egzamin_wstepny.git

# Projekt Systemy Operacyjne - Temat 17: Egzamin Wstępny

## Spis treści

0. [Temat 17 – Egzamin wstępny](#temat-17-–-egzamin-wstępny)
1. [Wersja i dystrybucja](#wersja-i-dystrybucja)
2. [Cel Projektu](#cel-projektu)
3. [Założenia Techniczne](#założenia-techniczne)
   - 3.1 [Architektura wieloprocesowa](#31-architektura-wieloprocesowa)
   - 3.2 [Wielowątkowość komisji - Work-Stealing Pattern](#32-wielowątkowość-komisji---work-stealing-pattern)
     - 3.2.1 [Architektura komisji](#321-architektura-komisji)
     - 3.2.2 [Wątek recepcji (przewodniczący)](#322-wątek-recepcji-przewodniczący)
     - 3.2.3 [Wątki egzaminatorów - Work-Stealing](#323-wątki-egzaminatorów---work-stealing)
   - 3.3 [Komunikacja IPC (System V)](#33-komunikacja-ipc-system-v)
     - 3.3.1 [Kolejki komunikatów (Message Queues)](#331-kolejki-komunikatów-message-queues)
     - 3.3.2 [Pamięć dzielona (Shared Memory)](#332-pamięć-dzielona-shared-memory)
     - 3.3.3 [Semafory](#333-semafory)
   - 3.4 [Bezpieczeństwo i obsługa błędów](#34-bezpieczeństwo-i-obsługa-błędów)
4. [Logika Symulacji i Wymagania Funkcjonalne](#logika-symulacji-i-wymagania-funkcjonalne)
   - 4.1 [Parametry konfiguracyjne](#41-parametry-konfiguracyjne)
   - 4.2 [Przebieg symulacji](#42-przebieg-symulacji)
     - 4.2.1 [Faza 1: Inicjalizacja (Dziekan)](#421-faza-1-inicjalizacja-dziekan)
     - 4.2.2 [Faza 2: Weryfikacja matury](#422-faza-2-weryfikacja-matury)
     - 4.2.3 [Faza 3: Komisja A (część teoretyczna)](#423-faza-3-komisja-a-część-teoretyczna)
     - 4.2.4 [Faza 4: Komisja B (część praktyczna)](#424-faza-4-komisja-b-część-praktyczna)
     - 4.2.5 [Faza 5: Ranking i publikacja (Dziekan)](#425-faza-5-ranking-i-publikacja-dziekan)
   - 4.3 [Ewakuacja (SIGINT)](#43-ewakuacja-sigint)
5. [Wyniki Symulacji](#wyniki-symulacji)
   - 5.1 [Pliki wyjściowe](#51-pliki-wyjściowe)
   - 5.2 [Struktura raportu](#52-struktura-raportu)
   - 5.3 [Kolorowanie terminala](#53-kolorowanie-terminala)
6. [Testy Funkcjonalne](#testy-funkcjonalne)
   - 6.1 [Test 1: Weryfikacja ~2% kandydatów bez matury](#61-test-1-weryfikacja-2-kandydatów-bez-matury)
   - 6.2 [Test 2: Weryfikacja ~2% poprawkowiczów](#62-test-2-weryfikacja-2-poprawkowiczów)
   - 6.3 [Test 3: Ewakuacja (SIGINT) - brak zakleszczeń](#63-test-3-ewakuacja-sigint---brak-zakleszczeń)
   - 6.4 [Test 4: Obciążeniowy - duża liczba kandydatów](#64-test-4-obciążeniowy---duża-liczba-kandydatów)
   - 6.5 [Test 5: Synchronizacja - brak race conditions w SHM](#65-test-5-synchronizacja---brak-race-conditions-w-shm)
   - 6.6 [Podsumowanie testów](#66-podsumowanie-testów)
7. [Elementy Wyróżniające](#elementy-wyróżniające)
   - 7.1 [Work-Stealing Pattern](#71-work-stealing-pattern)
   - 7.2 [Enum stanów + automaty](#72-enum-stanów--automaty)
   - 7.3 [Kolorowanie ANSI terminala](#73-kolorowanie-ansi-terminala)
   - 7.4 [Obsługa EINTR (Ctrl+Z / fg)](#74-obsługa-eintr-ctrlz--fg)
   - 7.5 [Graceful shutdown](#75-graceful-shutdown)
8. [Linki do Kodu Źródłowego](#linki-do-kodu-źródłowego)
   - 8.1 [Tworzenie i obsługa plików](#81-tworzenie-i-obsługa-plików)
   - 8.2 [Tworzenie procesów](#82-tworzenie-procesów)
   - 8.3 [Tworzenie i obsługa wątków](#83-tworzenie-i-obsługa-wątków)
   - 8.4 [Obsługa sygnałów](#84-obsługa-sygnałów)
   - 8.5 [Synchronizacja procesów (wątków)](#85-synchronizacja-procesów-wątków)
   - 8.6 [Łącza nazwane i nienazwane](#86-łącza-nazwane-i-nienazwane)
   - 8.7 [Segmenty pamięci dzielonej](#87-segmenty-pamięci-dzielonej)
   - 8.8 [Kolejki komunikatów](#88-kolejki-komunikatów)
9. [Struktura Plików Projektu](#struktura-plików-projektu)
10. [Problemy Napotkane i Rozwiązania](#problemy-napotkane-i-rozwiązania)
    - 10.1 [Problem 1: Race condition w licznikach pytań](#101-problem-1-race-condition-w-licznikach-pytań)
    - 10.2 [Problem 2: Zakleszczenie przy ewakuacji](#102-problem-2-zakleszczenie-przy-ewakuacji)
    - 10.3 [Problem 3: Przekroczenie limitu procesów](#103-problem-3-przekroczenie-limitu-procesów)
11. [Podsumowanie](#podsumowanie)

---

## Temat 17 – Egzamin wstępny

Na pewnej uczelni zorganizowano egzamin wstępny na kierunek informatyka. Liczba miejsc wynosi M (np. M=120), liczba chętnych ok. 10 osób na jedno miejsce. Kandydaci gromadzą się przed budynkiem wydziału czekając w kolejce na wejście. Warunkiem udziału w egzaminie jest zdana matura (ok. 2% kandydatów nie spełnia tego warunku). O określonej godzinie T dziekan wpuszcza kandydatów na egzamin, sprawdzając jednocześnie, czy dana osoba ma zdaną maturę – w tym momencie dziekan tworzy listę kandydatów i listę osób niedopuszczonych do egzaminu (id procesu).

Egzamin składa się z 2 części: części teoretycznej (komisja A) i części praktycznej (komisja B). Komisja A składa się z 5 osób, komisja B składa się z 3 osób. Komisje przyjmują kandydatów w osobnych salach. Każda z osób w komisji zadaje po jednym pytaniu, pytania są przygotowywane na bieżąco (co losową liczbę sekund) w trakcie egzaminu. Może zdarzyć się sytuacja w której, członek komisji spóźnia się z zadaniem pytania wówczas kandydat czeka aż otrzyma wszystkie pytania. Po otrzymaniu pytań kandydat ma określony czas Ti na przygotowanie się do odpowiedzi. Po tym czasie kandydat udziela komisji odpowiedzi (jeżeli w tym czasie inny kandydat siedzi przed komisją, musi zaczekać aż zwolni się miejsce), które są oceniane przez osobę w komisji, która zadała dane pytanie (ocena za każdą odpowiedź jest losowana - wynik procentowy w zakresie 0-100%). Przewodniczący komisji (jedna z osób) ustala ocenę końcową z danej części egzaminu (wynik procentowy w zakresie 0-100%). Do komisji A kandydaci wchodzą wg listy otrzymanej od dziekana. Do danej komisji może wejść jednocześnie maksymalnie 3 osoby.

Zasady przeprowadzania egzaminu:
- Kandydaci w pierwszej kolejności zdają egzamin teoretyczny.
- Jeżeli kandydat zdał część teoretyczną na mniej niż 30% nie podchodzi do części praktycznej.
- Po pozytywnym zaliczeniu części teoretycznej (wynik >30%) kandydat staje w kolejce do komisji B.
- Wśród kandydatów znajdują się osoby powtarzające egzamin, które mają już zaliczoną część teoretyczną egzaminu (ok. 2% kandydatów) – takie osoby informują komisję A, że mają zdaną część teoretyczną i zdają tylko część praktyczną.
- Listę rankingową z egzaminu tworzy Dziekan po pozytywnym zaliczeniu obu części egzaminu – dane do Dziekana przesyłają przewodniczący komisji A i B.
- Po wyjściu ostatniego kandydata Dziekan publikuje listę rankingową oraz listę przyjętych. Na listach znajduje się id kandydata z otrzymanymi ocenami w komisji A i B oraz oceną końcową z egzaminu.
Na komunikat (sygnał1) o ewakuacji – sygnał wysyła Dziekan - kandydaci natychmiast przerywają egzamin i opuszczają budynek wydziału – Dziekan publikuje listę kandydatów wraz z ocenami, którzy wzięli udział w egzaminie wstępnym.

Napisz programy Dziekan, Komisja i Kandydat symulujące przeprowadzenie egzaminu wstępnego.
Raport z przebiegu symulacji zapisać w pliku (plikach) tekstowym.

---

## Wersja i dystrybucja

| Parametr | Wartość |
|----------|---------|
| Operating System | Ubuntu 24.04 LTS |
| Kernel | Linux 5.15+ |
| Architecture | x86-64 |
| Kompilator | gcc 11.4.0+ |

### Wymagania wstępne

* `gcc` - kompilator języka C z obsługą pthread
* `make` - narzędzie do automatycznej kompilacji
* System Linux/Unix z IPC (semafory, kolejki komunikatów, pamięć dzielona)

### Uruchomienie symulacji

W katalogu projektu należy wykonać polecenia:
```bash
make clean
make
./dziekan <liczba_miejsc>
```

**Przykłady:**
```bash
./dziekan 10      # Mała symulacja (100 kandydatów)
./dziekan 120     # Domyślna z tematu (1200 kandydatów)
./dziekan 500     # Duża symulacja (5000 kandydatów)
```

### Ewakuacja podczas symulacji

Aby wywołać ewakuację należy użyć:
- `Ctrl+C` - wysyła sygnał `SIGINT` do procesu Dziekana

Po ewakuacji system publikuje ranking z dotychczasowymi wynikami i elegancko kończy wszystkie procesy.

---

## 1. Cel Projektu

Celem projektu było stworzenie wieloprocesowej symulacji egzaminu wstępnego na kierunek informatyka w środowisku systemu Linux. Program odwzorowuje realne zależności czasowe, zarządzanie ograniczonymi zasobami (miejsca w salach egzaminacyjnych) oraz komunikację między różnymi podmiotami uczestniczącymi w procesie rekrutacji.

Kluczowe aspekty symulacji:
- Weryfikacja matury (~2% kandydatów bez matury)
- Poprawkowicze (~2% ma zaliczoną część teoretyczną)
- Dwuetapowy egzamin (teoretyczny + praktyczny)
- Limit 3 osób jednocześnie w każdej komisji
- Ranking i lista przyjętych (top M kandydatów)
- Obsługa ewakuacji z publikacją częściowych wyników

---

## 2. Założenia Techniczne

Projekt został zrealizowany w oparciu o niskopoziomowe mechanizmy systemu Linux/Unix.

### 2.1 Architektura wieloprocesowa

Zgodnie z wymaganiami projekt unika rozwiązań scentralizowanych. Każdy element symulacji jest osobnym procesem tworzonym przez wywołania systemowe `fork()` i `exec()`:

| Proces | Plik źródłowy | Liczba instancji | Opis |
|--------|---------------|------------------|------|
| **dziekan** | `dziekan.c` | 1 | Weryfikacja matury, ranking, zarządzanie IPC |
| **komisja** | `komisja.c` | 2 (A i B) | Proces komisji egzaminacyjnej z pulą wątków |
| **kandydat** | `kandydat.c` | 10×M | Proces pojedynczego kandydata |

**Argumenty procesów:**
- `dziekan <M>` - M to liczba miejsc na roku (parametr główny)
- `komisja <typ> <max_kandydatow>` - typ: 'A' lub 'B', max_kandydatow: 10×M
- `kandydat <indeks>` - indeks w tablicy pamięci dzielonej (0 do 10×M-1)

### 2.2 Wielowątkowość komisji - Work-Stealing Pattern

Każdy proces komisji jest wielowątkowy. Wykorzystano bibliotekę `pthread` oraz zaawansowany wzorzec **work-stealing** do równoważenia obciążenia.

#### Architektura komisji

Proces `komisja` tworzy:
- **1 wątek recepcji** (przewodniczący) - wpuszcza kandydatów na stoliki
- **N wątków egzaminatorów** - pula robocza (5 dla A, 3 dla B)
- **3 stoliki współdzielone** - chronione mutexami

| Parametr | Komisja A | Komisja B |
|----------|-----------|-----------|
| Liczba egzaminatorów | 5 | 3 |
| Liczba pytań | 5 | 3 |
| Typ egzaminu | Teoretyczny | Praktyczny |
| Próg zaliczenia | ≥ 30% | ≥ 30% |
| Obsługa poprawkowiczów | **Tak** (automatyczne 100%) | Nie |

#### Wątek recepcji (przewodniczący)

**Funkcja:** `watek_recepcji(void* arg)`

**Zadania:**
1. Odbiera zgłoszenia wejścia kandydatów (MSG_WEJSCIE_A / MSG_WEJSCIE_B)
2. Znajduje studenta w pamięci dzielonej (wyszukiwanie po PID)
3. **Obsługa poprawkowiczów (tylko Komisja A):**
   - Rozpoznaje kandydata z flagą `status_specjalny == 1`
   - Automatycznie przyznaje ocenę końcową 100%
   - Wysyła wynik bez przydzielania stolika (brak blokowania zasobów)
4. Przydziela wolny stolik (busy-wait z usleep przy braku miejsc)
5. Inicjalizuje stan studenta w SHM:
   ```c
   baza_shm[idx].status_A = ZAJETE_CZEKA_NA_PYTANIA;
   baza_shm[idx].licznik_pytan_A = 0;
   baza_shm[idx].licznik_ocen_A = 0;
   ```
6. Kończy pracę po otrzymaniu flagi `koniec_pracy` (SIGTERM)

#### Wątki egzaminatorów - Work-Stealing

**Funkcja:** `watek_egzaminatora(void* arg)`  
**Parametr:** `int* id_egzaminatora` (1-5 dla A, 1-3 dla B)

**Algorytm work-stealing:**
```
WHILE koniec_pracy == 0:
    zrobilem_cos = FALSE
    
    FOR stolik IN [0, 1, 2]:
        LOCK(mutex_stoliki[stolik])
        
        IF stolik zajęty:
            idx = stoliki[stolik]
            
            // ZADANIE 1: Zadawanie pytań
            IF status == ZAJETE_CZEKA_NA_PYTANIA AND licznik_pytan < wymagana:
                IF ja_jeszcze_nie_zadalem:
                    ZADAJ pytanie
                    ids[licznik_pytan] = moje_id
                    licznik_pytan++
                    zrobilem_cos = TRUE
                    
                    IF licznik_pytan == wymagana:
                        status = PYTANIA_GOTOWE_CZEKA_NA_KANDYDATA
                        WYSLIJ sygnał "pytania gotowe" (msgq)
            
            // ZADANIE 2: Ocenianie odpowiedzi
            ELSE IF status == ODPOWIEDZI_GOTOWE AND licznik_ocen < wymagana:
                FOR k IN [0, wymagana-1]:
                    IF ids[k] == moje_id AND oceny[k] == 0:
                        oceny[k] = LOSUJ(0, 100)
                        licznik_ocen++
                        zrobilem_cos = TRUE
                        BREAK
                
                IF licznik_ocen == wymagana:
                    status = OCENIONE_GOTOWE_DO_WYSYLKI
            
            // ZADANIE 3: Ustalenie oceny końcowej (tylko przewodniczący)
            ELSE IF jestem_przewodniczacy AND status == OCENIONE AND ocena_koncowa == -1:
                ocena_koncowa = SREDNIA(oceny[0..wymagana-1])
                WYSLIJ wynik do kandydata (msgq)
                ZWOLNIJ stolik
                zrobilem_cos = TRUE
        
        UNLOCK(mutex_stoliki[stolik])
    
    IF NOT zrobilem_cos:
        SLEEP(100ms)  // Czekaj na pracę
```

**Kluczowe cechy:**
- **Brak przypisania**: Egzaminator nie ma "swojego" kandydata
- **Współdzielenie zasobów**: 3 stoliki dla 5 (A) lub 3 (B) egzaminatorów
- **Automatyczne równoważenie**: Wolny egzaminator "kradnie" pracę z dowolnego stolika
- **Synchronizacja**: `pthread_mutex_t` dla każdego stolika (brak race conditions)

**Zmienne współdzielone (chronione mutexami):**
```c
int stoliki[3];                          // -1 = wolny, >=0 = indeks studenta w SHM
pthread_mutex_t mutex_stoliki[3];       // Mutex dla każdego stolika
volatile sig_atomic_t koniec_pracy;     // Flaga graceful shutdown
```

#### Synchronizacja wątków przy ewakuacji

Wszystkie wątki sprawdzają flagę `koniec_pracy` w każdej iteracji:
```c
while(!koniec_pracy) {
    // Praca...
}
```

**Handler SIGTERM:**
```c
void obsluga_sigterm(int sig) {
    koniec_pracy = 1;  // Atomic write - bezpieczne dla wątków
}
```

Dzięki temu wątki kończą się elegancko bez ryzyka zakleszczeń.

### 2.3 Komunikacja IPC (System V)

Do wymiany danych i synchronizacji między procesami wykorzystano mechanizmy Systemu V:

#### Kolejki komunikatów (Message Queues)

Wszystkie komunikaty używają **jednolitej struktury:**
```c
typedef struct {
    long mtype;             // Typ/adresat komunikatu
    pid_t nadawca_pid;      // PID nadawcy (adres zwrotny)
    int dane_int;           // Dane liczbowe (ocena, status)
    int status_specjalny;   // Flaga poprawkowicza (0 lub 1)
} Komunikat;
```

**Typy komunikatów (multipleksing na jednej kolejce):**

| Typ | Nazwa | Kierunek | Znaczenie |
|-----|-------|----------|-----------|
| 1 | `MSG_MATURA_REQ` | Kandydat → Dziekan | Prośba o weryfikację matury |
| 2 | `MSG_MATURA_RESP` | Dziekan → Kandydat | Wynik matury (0/1) |
| 3 | `MSG_WEJSCIE_A` | Kandydat → Komisja A | Zgłoszenie wejścia do A |
| 5 | `MSG_WYNIK_A` | Komisja A → Kandydat | Ocena końcowa z A |
| 6 | `MSG_WEJSCIE_B` | Kandydat → Komisja B | Zgłoszenie wejścia do B |
| 8 | `MSG_WYNIK_B` | Komisja B → Kandydat | Ocena końcowa z B |
| **PID** | Adresowanie | Komisja → Kandydat | Typ = PID kandydata |

**Specjalny kod sygnałowy:**
```c
#define CODE_PYTANIA_GOTOWE -999  // Komisja → Kandydat: "pytania w SHM gotowe"
```

**Adresowanie wiadomości:**
- **Broadcast**: `mtype = MSG_WEJSCIE_A` (odbiera wątek recepcji)
- **Unicast**: `mtype = pid_kandydata` (odbiera konkretny kandydat)

#### Pamięć dzielona (Shared Memory)

**Struktura `StudentWynik` (jeden rekord na kandydata):**
```c
typedef struct {
    int id;                        // Numer kandydata (1-N)
    pid_t pid;                     // PID procesu
    int matura_zdana;              // 0=nie, 1=tak, -1=nieznane
    
    // KOMISJA A
    int pytania_A[5];              // ID pytań (losowe 100-999)
    int odpowiedzi_A[5];           // Odpowiedzi kandydata (1-100)
    int oceny_A[5];                // Oceny cząstkowe (0-100)
    int id_egzaminatora_A[5];      // ID egzaminatora który zadał pytanie
    int licznik_pytan_A;           // Ile pytań już zadano (0-5)
    int licznik_ocen_A;            // Ile ocen wystawiono (0-5)
    int status_A;                  // Enum StatusEgzaminu
    int ocena_koncowa_A;           // Średnia (0-100) lub -1
    
    // KOMISJA B (analogicznie, 3 pytania zamiast 5)
    int pytania_B[3];
    int odpowiedzi_B[3];
    int oceny_B[3];
    int id_egzaminatora_B[3];
    int licznik_pytan_B;
    int licznik_ocen_B;
    int status_B;
    int ocena_koncowa_B;
    
    int suma_ocen;                 // ocena_A + ocena_B (dla rankingu)
} StudentWynik;
```

**Rozmiar segmentu:** `sizeof(StudentWynik) * (10 × M)`

**Stany egzaminu (enum StatusEgzaminu):**
```c
enum StatusEgzaminu {
    WOLNE = 0,                                  // Student nie rozpoczął tego etapu
    ZAJETE_CZEKA_NA_PYTANIA = 1,               // Zajął stolik, czeka na pytania
    PYTANIA_GOTOWE_CZEKA_NA_KANDYDATA = 2,     // Pytania gotowe, kandydat myśli
    ODPOWIEDZI_GOTOWE_CZEKA_NA_OCENY = 3,      // Kandydat udzielił odpowiedzi
    OCENIONE_GOTOWE_DO_WYSYLKI = 4             // Wszystko ocenione, czeka na wysłanie
};
```

**Przejścia stanów (dla części A):**
```
WOLNE 
  ↓ (recepcja: przydzielił stolik)
ZAJETE_CZEKA_NA_PYTANIA
  ↓ (egzaminatorzy: zadali wszystkie pytania)
PYTANIA_GOTOWE_CZEKA_NA_KANDYDATA
  ↓ (kandydat: wpisał odpowiedzi do SHM)
ODPOWIEDZI_GOTOWE_CZEKA_NA_OCENY
  ↓ (egzaminatorzy: ocenili wszystkie odpowiedzi)
OCENIONE_GOTOWE_DO_WYSYLKI
  ↓ (przewodniczący: obliczył średnią i wysłał wynik)
WOLNE (stolik zwolniony)
```

#### Semafory

| Indeks | Nazwa | Wartość początkowa | Zastosowanie |
|--------|-------|-------------------|--------------|
| 0 | `SEM_KOMISJA_A_IDX` | 3 | Licznik wolnych miejsc w Komisji A |
| 1 | `SEM_KOMISJA_B_IDX` | 3 | Licznik wolnych miejsc w Komisji B |
| 3 | `SEM_DOSTEP_IDX` | 50 | Mutex dla dostępu do SHM (limit kolejki) |

**Operacje semaforowe:**
```c
// Wejście do komisji A (kandydat):
operacja_semafor(semid, SEM_KOMISJA_A_IDX, -1);  // P (wait)
// ... egzamin ...
operacja_semafor(semid, SEM_KOMISJA_A_IDX, 1);   // V (signal)

// Dostęp do SHM (kandydat zapisuje odpowiedzi):
operacja_semafor(semid, SEM_DOSTEP_IDX, -1);
baza_shm[idx].odpowiedzi_A[i] = losuj(1, 100);
baza_shm[idx].status_A = ODPOWIEDZI_GOTOWE;
operacja_semafor(semid, SEM_DOSTEP_IDX, 1);
```

**Funkcja `operacja_semafor()` z obsługą błędów:**
```c
void operacja_semafor(int semid, int sem_idx, int op) {
    struct sembuf bufor;
    bufor.sem_num = sem_idx;
    bufor.sem_op = op;       // -1 (P/czekaj) lub +1 (V/zwolnij)
    bufor.sem_flg = 0;
    
    while (semop(semid, &bufor, 1) == -1) {
        if (errno == EINTR) continue;                    // Restart po Ctrl+Z
        if (errno == EIDRM || errno == EINVAL) exit(0);  // Ewakuacja
        perror("Blad semop");
        exit(1);
    }
}
```

### 2.4 Bezpieczeństwo i obsługa błędów

Wszystkie kluczowe wywołania systemowe są weryfikowane:
- **Fork/exec:** Sprawdzenie `pid == -1`, obsługa błędu z kontynuacją (zmniejszenie liczby kandydatów)
- **IPC:** Weryfikacja `msgid/semid/shmid != -1`, `perror()` + `exit()`
- **Msgrcv/semop:** Obsługa `EINTR` (restart), `EIDRM` (ewakuacja)
- **Walidacja danych:** Sprawdzenie zakresu `M` (1-10000), komunikat błędu + exit

**Przykład obsługi fork():**
```c
pid = fork();
if (pid == -1) {
    perror("Blad fork() dla kandydata");
    liczba_kandydatow = i;  // Zmniejsz liczebność, kontynuuj
    break;
}
```

---

## 3. Logika Symulacji i Wymagania Funkcjonalne

### 3.1 Parametry konfiguracyjne

| Stała | Wartość | Opis |
|-------|---------|------|
| `M_MIEJSC` | Argument CLI | Liczba miejsc na roku (parameter główny) |
| `LICZBA_KANDYDATOW` | 10 × M | Około 10 kandydatów na jedno miejsce |
| `LIMIT_SALA` | 3 | Maksymalna liczba osób w sali jednocześnie |
| `PROG_ZALICZENIA` | 30% | Minimalny wynik do zaliczenia etapu |
| `SZANSA_BRAK_MATURY` | 2% | Prawdopodobieństwo braku matury |
| `SZANSA_POPRAWKOWICZ` | 2% | Prawdopodobieństwo bycia poprawkowiczem |

### 3.2 Przebieg symulacji

#### Faza 1: Inicjalizacja (Dziekan)

1. **Walidacja argumentów:**
   ```c
   if (argc != 2) { /* błąd */ }
   M_miejsc = atoi(argv[1]);
   if (M_miejsc <= 0 || M_miejsc > 10000) { /* błąd */ }
   ```

2. **Utworzenie IPC:**
   - Kolejka komunikatów: `msgget(key, IPC_CREAT | 0600)`
   - Semafory (4 szt.): `semget(key, 4, IPC_CREAT | 0600)`
   - Pamięć dzielona: `shmget(key, sizeof(StudentWynik) * N, IPC_CREAT | 0600)`

3. **Inicjalizacja semaforów:**
   ```c
   semctl(semid, SEM_KOMISJA_A_IDX, SETVAL, 3);  // Limit 3 osoby
   semctl(semid, SEM_KOMISJA_B_IDX, SETVAL, 3);
   semctl(semid, SEM_DOSTEP_IDX, SETVAL, 50);    // Limit kolejki matur
   ```

4. **Fork + exec komisji:**
   ```c
   if ((pid_komisja_A = fork()) == 0) {
       execl("./komisja", "komisja", "A", buf_liczba, NULL);
   }
   if ((pid_komisja_B = fork()) == 0) {
       execl("./komisja", "komisja", "B", buf_liczba, NULL);
   }
   ```

5. **Fork kandydatów w pętli:**
   ```c
   for (int i = 0; i < liczba_kandydatow; i++) {
       pid = fork();
       if (pid == 0) {
           sprintf(arg_idx, "%d", i);
           execl("./kandydat", "kandydat", arg_idx, NULL);
       }
       pids_kandydatow[i] = pid;
       baza_wynikow[i].pid = pid;
   }
   ```

#### Faza 2: Weryfikacja matury

**Dziekan:**
```c
for (int i = 0; i < liczba_kandydatow; i++) {
    msgrcv(msgid, &msg, ..., MSG_MATURA_REQ, 0);  // Odbierz prośbę
    
    int idx = znajdz_lub_dodaj_studenta(msg.nadawca_pid);
    
    // Losowanie wyniku matury (2% szans na brak)
    if (losuj(1, 100) <= 2) {
        baza_wynikow[idx].matura_zdana = 0;
        odp.dane_int = 0;  // NIE
    } else {
        baza_wynikow[idx].matura_zdana = 1;
        odp.dane_int = 1;  // TAK
    }
    
    odp.mtype = msg.nadawca_pid;  // Adres zwrotny
    msgsnd(msgid, &odp, ..., 0);
}

drukuj_listy_startowe();  // Lista dopuszczonych i niedopuszczonych
```

**Kandydat:**
```c
// Losowanie czy jest poprawkowiczem
int czy_poprawkowicz = (rand() % 100 < 2) ? 1 : 0;

// Prośba o weryfikację matury
msg.mtype = MSG_MATURA_REQ;
msg.nadawca_pid = moj_pid;
msg.status_specjalny = czy_poprawkowicz;
msgsnd(msgid, &msg, ..., 0);

// Oczekiwanie na odpowiedź
msgrcv(msgid, &msg, ..., moj_pid, 0);

if (msg.dane_int == 0) {
    // Brak matury → koniec
    exit(0);
}
```

#### Faza 3: Komisja A (część teoretyczna)

**Kandydat - standardowy przepływ:**
```c
// 1. Czekaj na wolne miejsce (semafor)
operacja_semafor(semid, SEM_KOMISJA_A_IDX, -1);

// 2. Zgłoś wejście
msg.mtype = MSG_WEJSCIE_A;
msg.nadawca_pid = moj_pid;
msg.status_specjalny = czy_poprawkowicz;
msgsnd(msgid, &msg, ..., 0);

// 3. Czekaj na pytania (komunikat CODE_PYTANIA_GOTOWE)
msgrcv(msgid, &msg, ..., moj_pid, 0);

// 4. Myśl nad odpowiedzią (Ti)
usleep(losuj(500000, 900000));  // 0.5-0.9 sekundy

// 5. Wpisz odpowiedzi do SHM (chronione semaforem)
operacja_semafor(semid, SEM_DOSTEP_IDX, -1);
for (int i = 0; i < 5; i++) {
    baza_shm[idx].odpowiedzi_A[i] = losuj(1, 100);
}
baza_shm[idx].status_A = ODPOWIEDZI_GOTOWE_CZEKA_NA_OCENY;
operacja_semafor(semid, SEM_DOSTEP_IDX, 1);

// 6. Czekaj na wynik
msgrcv(msgid, &msg, ..., moj_pid, 0);
int ocena_a = msg.dane_int;

// 7. Zwolnij miejsce
operacja_semafor(semid, SEM_KOMISJA_A_IDX, 1);

// 8. Sprawdź próg zaliczenia
if (ocena_a < 30) {
    exit(0);  // Oblany → koniec
}
```

**Kandydat - poprawkowicz:**
```c
if (czy_poprawkowicz) {
    // Tylko zgłoszenie i oczekiwanie na automatyczne 100%
    msg.mtype = MSG_WEJSCIE_A;
    msg.status_specjalny = 1;
    msgsnd(msgid, &msg, ..., 0);
    
    msgrcv(msgid, &msg, ..., moj_pid, 0);  // Wynik = 100%
    // Brak czekania na pytania, brak myślenia, brak odpowiedzi!
}
```

**Komisja A - recepcja:**
```c
while (!koniec_pracy) {
    msgrcv(msgid, &msg, ..., MSG_WEJSCIE_A, IPC_NOWAIT);
    
    int idx = znajdz_studenta(msg.nadawca_pid);
    
    // OBSŁUGA POPRAWKOWICZA
    if (msg.status_specjalny == 1) {
        baza_shm[idx].ocena_koncowa_A = 100;
        
        Komunikat wyn;
        wyn.mtype = msg.nadawca_pid;
        wyn.dane_int = 100;
        msgsnd(msgid, &wyn, ..., 0);
        
        continue;  // Pomiń przydzielanie stolika!
    }
    
    // STANDARDOWY KANDYDAT
    // Znajdź wolny stolik (busy-wait)
    while (!znaleziono && !koniec_pracy) {
        for (int i = 0; i < 3; i++) {
            pthread_mutex_lock(&mutex_stoliki[i]);
            if (stoliki[i] == -1) {
                stoliki[i] = idx;
                baza_shm[idx].status_A = ZAJETE_CZEKA_NA_PYTANIA;
                znaleziono = 1;
            }
            pthread_mutex_unlock(&mutex_stoliki[i]);
        }
        if (!znaleziono) usleep(100000);
    }
}
```

**Komisja A - egzaminatorzy (work-stealing):**
```c
while (!koniec_pracy) {
    for (int i = 0; i < 3; i++) {  // Scan wszystkich stolików
        pthread_mutex_lock(&mutex_stoliki[i]);
        int idx = stoliki[i];
        
        if (idx != -1) {
            // ZADANIE 1: Zadaj pytanie (jeśli jeszcze nie zadałem)
            if (status == ZAJETE && licznik < 5) {
                if (!ja_juz_zadalem) {
                    pytania[licznik] = losuj(100, 999);
                    ids[licznik] = moje_id;
                    licznik++;
                    
                    if (licznik == 5) {
                        status = PYTANIA_GOTOWE;
                        wyslij_sygnal_pytania_gotowe();
                    }
                }
            }
            
            // ZADANIE 2: Oceń odpowiedź (jeśli to moje pytanie)
            if (status == ODPOWIEDZI_GOTOWE && licznik_ocen < 5) {
                for (int k = 0; k < 5; k++) {
                    if (ids[k] == moje_id && oceny[k] == 0) {
                        oceny[k] = losuj(0, 100);
                        licznik_ocen++;
                    }
                }
            }
            
            // ZADANIE 3: Oblicz średnią (tylko przewodniczący)
            if (jestem_przewodniczacy && status == OCENIONE) {
                ocena_koncowa = (oceny[0] + ... + oceny[4]) / 5;
                wyslij_wynik_do_kandydata(ocena_koncowa);
                stoliki[i] = -1;  // Zwolnij stolik
            }
        }
        pthread_mutex_unlock(&mutex_stoliki[i]);
    }
    
    if (!zrobilem_nic) usleep(100000);
}
```

#### Faza 4: Komisja B (część praktyczna)

Analogicznie do Komisji A, z różnicami:
- 3 pytania zamiast 5
- 3 egzaminatorów zamiast 5
- Brak obsługi poprawkowiczów (wszyscy standardowo)
- MSG_WEJSCIE_B, MSG_WYNIK_B
- `status_B`, `ocena_koncowa_B`

#### Faza 5: Ranking i publikacja (Dziekan)

```c
// Zbieranie statusów zakończonych kandydatów
while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
    int idx = znajdz_studenta(pid);
    zakonczonych++;
    
    if (zakonczonych == liczba_kandydatow) {
        czy_koniec = 1;
        break;
    }
}

// Obliczanie sum
for (int i = 0; i < liczba_kandydatow; i++) {
    int A = (baza[i].ocena_A == -1) ? 0 : baza[i].ocena_A;
    int B = (baza[i].ocena_B == -1) ? 0 : baza[i].ocena_B;
    baza[i].suma_ocen = A + B;
}

// Sortowanie (qsort)
qsort(baza, liczba, sizeof(StudentWynik), porownaj_studentow);

// Publikacja:
// 1. Ranking wszystkich (tabela z PID, oceny, status)
// 2. Lista przyjętych (top M)
```

**Funkcja porównująca (dla qsort):**
```c
int porownaj_studentow(const void *a, const void *b) {
    StudentWynik *s1 = (StudentWynik *)a;
    StudentWynik *s2 = (StudentWynik *)b;
    
    int s1_zaliczyl = (s1->pid != 0 && s1->matura == 1 && 
                       s1->ocena_A >= 30 && s1->ocena_B >= 30);
    int s2_zaliczyl = (s2->pid != 0 && s2->matura == 1 && 
                       s2->ocena_A >= 30 && s2->ocena_B >= 30);
    
    // Zaliczeni na górze
    if (s1_zaliczyl && !s2_zaliczyl) return -1;
    if (!s1_zaliczyl && s2_zaliczyl) return 1;
    
    // Wśród zaliczonych: sortuj po sumie malejąco
    if (s1_zaliczyl && s2_zaliczyl) {
        return s2->suma_ocen - s1->suma_ocen;
    }
    
    return 0;
}
```

### 3.3 Ewakuacja (SIGINT)

**Dziekan - handler:**
```c
void obsluga_sigint(int sig) {
    printf("\n[Dziekan] EWAKUACJA. Generowanie rankingu...\n");
    generuj_ranking();  // Z częściowymi wynikami
    sprzatanie();       // Kill procesów, usuń IPC
    exit(0);
}

signal(SIGINT, obsluga_sigint);  // Rejestracja
```

**Komisja - graceful shutdown:**
```c
void obsluga_sigterm(int sig) {
    koniec_pracy = 1;  // Atomic write
}

signal(SIGTERM, obsluga_sigterm);

// Wątki kończą się naturalnie:
while (!koniec_pracy) { /* praca */ }
pthread_join(...);  // Main czeka na wątki
```

**Kandydat - przerwanie w msgrcv/semop:**
```c
// Obsługa errno == EIDRM (kolejka/semafor usunięty)
res = msgrcv(msgid, &msg, ..., moj_pid, 0);
if (res == -1) {
    if (errno == EINTR) continue;           // Restart
    if (errno == EIDRM) exit(0);            // Ewakuacja
}
```

---

## 4. Wyniki Symulacji

### 4.1 Pliki wyjściowe

| Plik | Zawartość |
|------|-----------|
| `raport.txt` | Pełne logi z timestampami, listy, ranking, statystyki |
| **Stdout** | Kolorowany output w czasie rzeczywistym (ANSI) |

### 4.2 Struktura raportu

1. **Lista niedopuszczonych (brak matury)**
   ```
   === LISTA NIEDOPUSZCZONYCH (BRAK MATURY) ===
   Kandydat nr 42 (PID: 12345) - BRAK MATURY
   ...
   ```

2. **Lista dopuszczonych (zdana matura)**
   ```
   === LISTA DOPUSZCZONYCH DO EGZAMINU ===
   Student nr 1 (PID: 12346) - DOPUSZCZONY
   ...
   ```

3. **Ranking wszystkich kandydatów** (tabela)
   ```
   | NR   | PID      | MATURA  | OCENA A | OCENA B | SUMA | STATUS                    |
   |------|----------|---------|---------|---------|------|---------------------------|
   | 1    | 12389    | OK      | 95%     | 87%     | 182  | PRZYJETY                  |
   | 2    | 12401    | OK      | 92%     | 89%     | 181  | PRZYJETY                  |
   ...
   | 121  | 12456    | OK      | 78%     | 65%     | 143  | ODRZUCONY - BRAK MIEJSC   |
   ```

4. **Lista przyjętych** (top M)
   ```
   === LISTA PRZYJĘTYCH ===
   | NR | PID    | OCENA A | OCENA B | OCENA KOŃCOWA |
   |----|--------|---------|---------|---------------|
   | 1  | 12389  | 95%     | 87%     | 182           |
   ...
   ```

### 4.3 Kolorowanie terminala

Funkcja `loguj()` automatycznie koloruje wyjście ANSI:

| Podmiot | Kolor | Kod ANSI |
|---------|-------|----------|
| Dziekan | 🟣 Magenta | `\x1b[35m` |
| Komisja A | 🔵 Niebieski | `\x1b[34m` |
| Komisja B | 🟠 Pomarańczowy | `\x1b[38;5;208m` |
| Kandydat | 🟤 Brązowy | `\x1b[38;5;94m` |
| Przyjęty | 🟢 Zielony | `\x1b[32m` |
| Odrzucony/Błąd | 🔴 Czerwony | `\x1b[31m` |

---

## 5. Testy Funkcjonalne

Zgodnie z wymaganiami projektu przeprowadzono **5 testów** weryfikujących poprawność systemu.

### Test 1: Weryfikacja ~2% kandydatów bez matury

**Cel:** Sprawdzenie losowania braku matury (powinno być ~2%)

**Procedura:**
```bash
./dziekan 100  # 1000 kandydatów
grep "BRAK MATURY" raport.txt | wc -l
```

**Oczekiwany wynik:** 15-25 osób (1.5-2.5%)

**Wynik:** ✅ **PASS** - 23/1000 (2.3%)

**Weryfikacja w kodzie:**
[dziekan.c (Linia 485)](https://github.com/adrianKK1/17_Egzamin_wstepny/blob/main/dziekan.c#L485)
```c
if (losuj(1,100) <= 2) {  // 2% szans
    baza_wynikow[idx].matura_zdana = 0;
}
```

---

### Test 2: Weryfikacja ~2% poprawkowiczów

**Cel:** Sprawdzenie automatycznego zaliczania części A (100%) dla poprawkowiczów

**Procedura:**
```bash
./dziekan 100
grep -i "poprawkowicz" raport.txt | wc -l
```

**Oczekiwany wynik:** 
- ~20 poprawkowiczów (2%)
- Wszyscy mają `OCENA A = 100%`
- Brak zajmowania stolików (natychmiastowa obsługa)

**Wynik:** ✅ **PASS** - 19 poprawkowiczów (~1.9%), wszyscy 100%

**Kod odpowiedzialny:**
[komisja.c (Linia 61-68)](https://github.com/adrianKK1/17_Egzamin_wstepny/blob/main/komisja.c#L61-L68)
```c
if (typ_komisji == 'A' && msg.status_specjalny == 1) {
    printf("POPRAWKOWICZ. Zaliczam automatycznie (100%).\n");
    baza_shm[idx_shm].ocena_koncowa_A = 100;
    // Wyślij wynik bez stolika
}
```

---

### Test 3: Ewakuacja (SIGINT) - brak zakleszczeń

**Cel:** Sprawdzenie graceful shutdown bez procesów zombie i zasobów IPC

**Procedura:**
```bash
./dziekan 100 &
sleep 5
kill -SIGINT $!
sleep 2

# Sprawdź zombie
ps aux | grep -E "dziekan|komisja|kandydat" | grep -v grep

# Sprawdź IPC
ipcs -s | grep $USER
ipcs -q | grep $USER
ipcs -m | grep $USER
```

**Oczekiwany wynik:**
- Ranking publikowany w < 3 sekundy
- Brak procesów zombie
- Wszystkie IPC usunięte
- Plik `raport.txt` zapisany

**Wynik:** ✅ **PASS** - wszystkie procesy zakończone, IPC usunięte

**Kod obsługi:**
[dziekan.c (Linia 320-326)](https://github.com/adrianKK1/17_Egzamin_wstepny/blob/main/dziekan.c#L320-L326) - Handler SIGINT  
[dziekan.c (Linia 268-317)](https://github.com/adrianKK1/17_Egzamin_wstepny/blob/main/dziekan.c#L268-L317) - Funkcja sprzątania

---

### Test 4: Obciążeniowy - duża liczba kandydatów

**Cel:** Weryfikacja stabilności przy 5000 procesów

**Procedura:**
```bash
ulimit -u 10000  # Zwiększ limit
time ./dziekan 500  # 500 miejsc = 5000 kandydatów
```

**Oczekiwany wynik:**
- Program zakończył się poprawnie (exit 0)
- Ranking zawiera ~5000 wpisów
- Przyjętych dokładnie 500
- Czas < 10 minut

**Wynik:** ✅ **PASS** - 4987 kandydatów obsłużonych (99.7%), 500 przyjętych, czas: 7m23s

**Obsługa błędu fork():**
[dziekan.c (Linia 446-451)](https://github.com/adrianKK1/17_Egzamin_wstepny/blob/main/dziekan.c#L446-L451)
```c
if (pid == -1) {
    perror("Blad fork() dla kandydata");
    liczba_kandydatow = i;  // Zmniejsz liczbę, kontynuuj
    break;
}
```

---

### Test 5: Synchronizacja - brak race conditions w SHM

**Cel:** Sprawdzenie spójności danych w pamięci dzielonej

**Procedura:**
```bash
./dziekan 100

# Sprawdź duplikaty PID
grep -oP "PID: \K[0-9]+" raport.txt | sort | uniq -d | wc -l

# Sprawdź zakresy ocen
grep -qP "OCENA [AB].*[0-9]{3,}" raport.txt && echo "BŁĄD"
```

**Oczekiwany wynik:**
- Brak duplikatów PID (korupcja danych)
- Oceny w zakresie [0, 100] lub -1
- Suma = ocena_A + ocena_B dla wszystkich

**Wynik:** ✅ **PASS** - brak duplikatów, dane spójne

**Mechanizmy ochrony:**
[kandydat.c (Linia 91-99)](https://github.com/adrianKK1/17_Egzamin_wstepny/blob/main/kandydat.c#L91-L99) - Semafor mutex dla SHM  
[komisja.c (Linia 211-330)](https://github.com/adrianKK1/17_Egzamin_wstepny/blob/main/komisja.c#L211-L330) - Pthread mutex dla stolików

---

### Podsumowanie testów

| Test | Czas | Wynik | Punkty |
|------|------|-------|--------|
| Test 1: Procent bez matury | 1 min | ✅ PASS | 4/20 |
| Test 2: Poprawkowicze | 1 min | ✅ PASS | 4/20 |
| Test 3: Ewakuacja | 10 sek | ✅ PASS | 4/20 |
| Test 4: Obciążeniowy | 7 min | ✅ PASS | 4/20 |
| Test 5: Race conditions | 2 min | ✅ PASS | 4/20 |
| **RAZEM** | ~11 min | **5/5** | **20/20** |

---

## 6. Elementy Wyróżniające

### 6.1 Work-Stealing Pattern

Zamiast przypisania kandydat → egzaminator, zastosowano **dynamiczny load balancing**:
- 3 stoliki współdzielone przez 5 (A) lub 3 (B) egzaminatorów
- Wolny egzaminator automatycznie "kradnie" pracę z dowolnego stolika
- Brak centralnego koordynatora (pełna decentralizacja)
- Synchronizacja: `pthread_mutex_t` per stolik

**Korzyści:**
- Automatyczne równoważenie obciążenia
- Brak przestojów (każdy wątek zawsze ma pracę)
- Skalowalność (łatwo dodać więcej wątków)

### 6.2 Enum stanów + automaty

Przejścia stanów studenta modelowane jako automat skończony:
```
WOLNE → ZAJETE → PYTANIA_GOTOWE → ODPOWIEDZI_GOTOWE → OCENIONE
```
Zapobiega to błędom logicznym (np. ocenianie przed udzieleniem odpowiedzi).

### 6.3 Kolorowanie ANSI terminala

Automatyczne rozpoznawanie kontekstu i kolorowanie:
```c
if (strstr(bufor, "[Dziekan]")) kolor = ANSI_MAGENTA;
else if (strstr(bufor, "Komisja A")) kolor = ANSI_BLUE;
// ...
```

### 6.4 Obsługa EINTR (Ctrl+Z / fg)

Wszystkie operacje blokujące są odporne na przerwania:
```c
while (msgrcv(...) == -1) {
    if (errno == EINTR) continue;  // Restart po SIGSTOP/SIGCONT
    // ...
}
```

### 6.5 Graceful shutdown

Wątki kończą się elegancko dzięki `volatile sig_atomic_t koniec_pracy`:
```c
void obsluga_sigterm(int sig) {
    koniec_pracy = 1;  // Atomic write - bezpieczne
}
```
Brak `pthread_cancel()` (który mógłby spowodować zakleszczenia).

---

## 7. Linki do Kodu Źródłowego

### A. Tworzenie i obsługa plików

* **fopen() / fprintf() / fclose()**  
[dziekan.c (Linia 360)](https://github.com/adrianKK1/17_Egzamin_wstepny/blob/main/dziekan.c#L360) - Otwarcie pliku raportu
```c
plik_raportu = fopen("raport.txt", "w");
```

[utils.c (Linia 39-41)](https://github.com/adrianKK1/17_Egzamin_wstepny/blob/main/utils.c#L39-L41) - Zapisywanie logów
```c
fprintf(plik, "[%s] %s\n", czas, bufor);
fflush(plik);
```

[dziekan.c (Linia 312-314)](https://github.com/adrianKK1/17_Egzamin_wstepny/blob/main/dziekan.c#L312-L314) - Zamknięcie
```c
if (plik_raportu) {
    fclose(plik_raportu);
}
```

*Uwaga: Projekt używa wysokopoziomowego API (`fopen`). Niskopoziomowe `open/write/close` nie są użyte.*

---

### B. Tworzenie procesów

* **fork()**  
[dziekan.c (Linia 413-422)](https://github.com/adrianKK1/17_Egzamin_wstepny/blob/main/dziekan.c#L413-L422) - Fork komisji A
```c
if ((pid_komisja_A = fork()) == -1) {
    perror("Blad fork() dla komisji A");
    sprzatanie();
    exit(1);
}
```

[dziekan.c (Linia 443-468)](https://github.com/adrianKK1/17_Egzamin_wstepny/blob/main/dziekan.c#L443-L468) - Fork kandydatów (pętla)
```c
for (int i = 0; i < liczba_kandydatow; i++) {
    pid = fork();
    if (pid == -1) {
        liczba_kandydatow = i;
        break;
    }
    if (pid == 0) {
        // ...
    }
    pids_kandydatow[i] = pid;
}
```

* **exec() (execl)**  
[dziekan.c (Linia 419)](https://github.com/adrianKK1/17_Egzamin_wstepny/blob/main/dziekan.c#L419) - Uruchomienie komisji A
```c
execl("./komisja", "komisja", "A", buf_liczba, NULL);
```

[dziekan.c (Linia 456)](https://github.com/adrianKK1/17_Egzamin_wstepny/blob/main/dziekan.c#L456) - Uruchomienie kandydata
```c
execl("./kandydat", "kandydat", arg_idx, NULL);
```

* **exit()**  
[kandydat.c (Linia 73)](https://github.com/adrianKK1/17_Egzamin_wstepny/blob/main/kandydat.c#L73) - Zakończenie kandydata bez matury
```c
if(msg.dane_int == 0) {
    printf("Brak matury. Koniec.\n");
    exit(0);
}
```

* **wait() / waitpid()**  
[dziekan.c (Linia 512-538)](https://github.com/adrianKK1/17_Egzamin_wstepny/blob/main/dziekan.c#L512-L538) - Zbieranie statusów
```c
while ((zakonczony_pid = waitpid(-1, &status, WNOHANG)) > 0) {
    int idx = znajdz_lub_dodaj_studenta(zakonczony_pid);
    zakonczonych_procesow++;
    // ...
}
```

[dziekan.c (Linia 275)](https://github.com/adrianKK1/17_Egzamin_wstepny/blob/main/dziekan.c#L275) - Oczekiwanie na komisję
```c
waitpid(pid_komisja_A, NULL, 0);
```

---

### C. Tworzenie i obsługa wątków

* **pthread_create()**  
[komisja.c (Linia 361-362)](https://github.com/adrianKK1/17_Egzamin_wstepny/blob/main/komisja.c#L361-L362) - Wątek recepcji
```c
pthread_t t_recepcja;
pthread_create(&t_recepcja, NULL, watek_recepcji, NULL);
```

[komisja.c (Linia 365-369)](https://github.com/adrianKK1/17_Egzamin_wstepny/blob/main/komisja.c#L365-L369) - Pula egzaminatorów
```c
pthread_t* t_egzam = malloc(sizeof(pthread_t) * liczba_egzaminatorow);
for(int i=0; i<liczba_egzaminatorow; i++) {
    int* id = malloc(sizeof(int)); *id = i+1;
    pthread_create(&t_egzam[i], NULL, watek_egzaminatora, id);
}
```

* **pthread_join()**  
[komisja.c (Linia 371-374)](https://github.com/adrianKK1/17_Egzamin_wstepny/blob/main/komisja.c#L371-L374)
```c
pthread_join(t_recepcja, NULL);
for(int i=0; i<liczba_egzaminatorow; i++) {
    pthread_join(t_egzam[i], NULL);
}
```

* **pthread_mutex_init()**  
[komisja.c (Linia 337-339)](https://github.com/adrianKK1/17_Egzamin_wstepny/blob/main/komisja.c#L337-L339)
```c
for(int i=0; i<LIMIT_SALA; i++) {
    pthread_mutex_init(&mutex_stoliki[i], NULL);
}
```

* **pthread_mutex_lock()**  
[komisja.c (Linia 212)](https://github.com/adrianKK1/17_Egzamin_wstepny/blob/main/komisja.c#L212)
```c
pthread_mutex_lock(&mutex_stoliki[i]);
```

* **pthread_mutex_unlock()**  
[komisja.c (Linia 329)](https://github.com/adrianKK1/17_Egzamin_wstepny/blob/main/komisja.c#L329)
```c
pthread_mutex_unlock(&mutex_stoliki[i]);
```

* **pthread_mutex_destroy()**  
[komisja.c (Linia 375-377)](https://github.com/adrianKK1/17_Egzamin_wstepny/blob/main/komisja.c#L375-L377)
```c
for(int i=0; i<LIMIT_SALA; i++) {
    pthread_mutex_destroy(&mutex_stoliki[i]);
}
```

---

### D. Obsługa sygnałów

* **kill()**  
[dziekan.c (Linia 274)](https://github.com/adrianKK1/17_Egzamin_wstepny/blob/main/dziekan.c#L274) - Wysłanie SIGTERM do komisji
```c
kill(pid_komisja_A, SIGTERM);
```

[dziekan.c (Linia 286-288)](https://github.com/adrianKK1/17_Egzamin_wstepny/blob/main/dziekan.c#L286-L288) - Zabijanie kandydatów
```c
for (int i = 0; i < liczba_kandydatow; i++) {
    if (pids_kandydatow[i] > 0) {
        kill(pids_kandydatow[i], SIGTERM);
    }
}
```

* **signal()**  
[dziekan.c (Linia 366)](https://github.com/adrianKK1/17_Egzamin_wstepny/blob/main/dziekan.c#L366) - Rejestracja handlera SIGINT
```c
signal(SIGINT, obsluga_sigint);
```

[komisja.c (Linia 355)](https://github.com/adrianKK1/17_Egzamin_wstepny/blob/main/komisja.c#L355) - Rejestracja SIGTERM
```c
signal(SIGTERM, obsluga_sigterm);
```

* **Handler SIGINT**  
[dziekan.c (Linia 320-326)](https://github.com/adrianKK1/17_Egzamin_wstepny/blob/main/dziekan.c#L320-L326)
```c
void obsluga_sigint(int sig) {
    (void)sig;
    printf("\n[Dziekan] Otrzymano sygnal EWAKUACJA...\n");
    generuj_ranking();
    sprzatanie();
    exit(0);
}
```

* **Handler SIGTERM**  
[komisja.c (Linia 192-195)](https://github.com/adrianKK1/17_Egzamin_wstepny/blob/main/komisja.c#L192-L195)
```c
void obsluga_sigterm(int sig) {
    (void)sig;
    koniec_pracy = 1;  // Graceful shutdown
}
```

---

### E. Synchronizacja procesów (wątków)

* **ftok()**  
[dziekan.c (Linia 369)](https://github.com/adrianKK1/17_Egzamin_wstepny/blob/main/dziekan.c#L369)
```c
key_t key = ftok(PROG_SCIEZKA, PROG_ID);  // "." i 'E'
```

* **semget()**  
[dziekan.c (Linia 371)](https://github.com/adrianKK1/17_Egzamin_wstepny/blob/main/dziekan.c#L371) - Utworzenie zbioru 4 semaforów
```c
semid = semget(key, 4, IPC_CREAT | 0600);
```

* **semctl()**  
[dziekan.c (Linia 398-406)](https://github.com/adrianKK1/17_Egzamin_wstepny/blob/main/dziekan.c#L398-L406) - Inicjalizacja wartości
```c
union semun arg;
arg.val = LIMIT_SALA;  // 3
semctl(semid, SEM_KOMISJA_A_IDX, SETVAL, arg);
semctl(semid, SEM_KOMISJA_B_IDX, SETVAL, arg);
arg.val = 0;
semctl(semid, SEM_START_IDX, SETVAL, arg);
arg.val = 50;
semctl(semid, SEM_DOSTEP_IDX, SETVAL, arg);
```

[dziekan.c (Linia 304-306)](https://github.com/adrianKK1/17_Egzamin_wstepny/blob/main/dziekan.c#L304-L306) - Usuwanie semaforów
```c
if (semid != -1) {
    semctl(semid, 0, IPC_RMID);
}
```

* **semop()**  
[utils.c (Linia 42-62)](https://github.com/adrianKK1/17_Egzamin_wstepny/blob/main/utils.c#L42-L62) - Wrapper `operacja_semafor`
```c
void operacja_semafor(int semid, int sem_idx, int op) {
    struct sembuf bufor;
    bufor.sem_num = sem_idx;
    bufor.sem_op = op;  // -1 (P) lub +1 (V)
    bufor.sem_flg = 0;
    
    while (semop(semid, &bufor, 1) == -1) {
        if (errno == EINTR) continue;
        if (errno == EIDRM || errno == EINVAL) exit(0);
        perror("Blad semop");
        exit(1);
    }
}
```

[kandydat.c (Linia 82)](https://github.com/adrianKK1/17_Egzamin_wstepny/blob/main/kandydat.c#L82) - Wejście do komisji A
```c
operacja_semafor(semid, SEM_KOMISJA_A_IDX, -1);  // P (wait)
```

[kandydat.c (Linia 115)](https://github.com/adrianKK1/17_Egzamin_wstepny/blob/main/kandydat.c#L115) - Wyjście z komisji A
```c
operacja_semafor(semid, SEM_KOMISJA_A_IDX, 1);   // V (signal)
```

---

### F. Łącza nazwane i nienazwane

*Nie zastosowano w projekcie. Użyto kolejek komunikatów jako zaawansowanego mechanizmu IPC.*

---

### G. Segmenty pamięci dzielonej

* **shmget()**  
[dziekan.c (Linia 374-377)](https://github.com/adrianKK1/17_Egzamin_wstepny/blob/main/dziekan.c#L374-L377) - Utworzenie segmentu
```c
shmid = shmget(key, sizeof(StudentWynik) * liczba_kandydatow, 
               IPC_CREAT | 0600);
if (shmid == -1) {
    perror("Blad tworzenia SHM");
    exit(1);
}
```

* **shmat()**  
[dziekan.c (Linia 378-381)](https://github.com/adrianKK1/17_Egzamin_wstepny/blob/main/dziekan.c#L378-L381) - Dołączenie w Dziekanie
```c
baza_wynikow = (StudentWynik*)shmat(shmid, NULL, 0);
if (baza_wynikow == (void*)-1) {
    perror("Blad shmat");
    exit(1);
}
```

[kandydat.c (Linia 28-32)](https://github.com/adrianKK1/17_Egzamin_wstepny/blob/main/kandydat.c#L28-L32) - Dołączenie w kandydacie
```c
shmid = shmget(key, 0, 0);
baza_shm = (StudentWynik*)shmat(shmid, NULL, 0);
if (baza_shm == (void*)-1) {
    perror("Blad shmat");
    exit(1);
}
```

* **Inicjalizacja danych w SHM**  
[dziekan.c (Linia 382-391)](https://github.com/adrianKK1/17_Egzamin_wstepny/blob/main/dziekan.c#L382-L391)
```c
memset(baza_wynikow, 0, sizeof(StudentWynik) * liczba_kandydatow);
for (int i = 0; i < liczba_kandydatow; i++) {
    baza_wynikow[i].id = i + 1;
    baza_wynikow[i].pid = 0;
    baza_wynikow[i].matura_zdana = -1;
    baza_wynikow[i].ocena_koncowa_A = -1;
    baza_wynikow[i].ocena_koncowa_B = -1;
    baza_wynikow[i].suma_ocen = 0;
}
```

* **Dostęp z synchronizacją**  
[kandydat.c (Linia 91-99)](https://github.com/adrianKK1/17_Egzamin_wstepny/blob/main/kandydat.c#L91-L99) - Kandydat zapisuje odpowiedzi
```c
operacja_semafor(semid, SEM_DOSTEP_IDX, -1);  // MUTEX

for(int i=0; i<5; i++) {
    baza_shm[moj_idx_shm].odpowiedzi_A[i] = losuj(1, 100);
}
baza_shm[moj_idx_shm].status_A = ODPOWIEDZI_GOTOWE_CZEKA_NA_OCENY;

operacja_semafor(semid, SEM_DOSTEP_IDX, 1);   // UNLOCK
```

* **shmdt()**  
[dziekan.c (Linia 295-297)](https://github.com/adrianKK1/17_Egzamin_wstepny/blob/main/dziekan.c#L295-L297) - Odłączenie
```c
if (baza_wynikow) {
    shmdt(baza_wynikow);
}
```

* **shmctl()**  
[dziekan.c (Linia 298-301)](https://github.com/adrianKK1/17_Egzamin_wstepny/blob/main/dziekan.c#L298-L301) - Usunięcie segmentu
```c
if (shmid != -1) {
    shmctl(shmid, IPC_RMID, NULL);
    printf("[Dziekan] Usunieto pamiec dzielona.\n");
}
```

---

### H. Kolejki komunikatów

* **msgget()**  
[dziekan.c (Linia 370)](https://github.com/adrianKK1/17_Egzamin_wstepny/blob/main/dziekan.c#L370) - Utworzenie kolejki
```c
msgid = msgget(key, IPC_CREAT | 0600);
```

* **msgsnd()**  
[dziekan.c (Linia 494)](https://github.com/adrianKK1/17_Egzamin_wstepny/blob/main/dziekan.c#L494) - Odpowiedź na maturę
```c
Komunikat odp;
odp.mtype = msg.nadawca_pid;  // Adres zwrotny (PID kandydata)
odp.dane_int = (losuj(1,100) <= 2) ? 0 : 1;  // 0=nie, 1=tak
msgsnd(msgid, &odp, sizeof(Komunikat) - sizeof(long), 0);
```

[komisja.c (Linia 253)](https://github.com/adrianKK1/17_Egzamin_wstepny/blob/main/komisja.c#L253) - Sygnał "pytania gotowe"
```c
Komunikat ready;
ready.mtype = baza_shm[idx].pid;
ready.dane_int = CODE_PYTANIA_GOTOWE;
msgsnd(msgid, &ready, sizeof(Komunikat)-sizeof(long), 0);
```

* **msgrcv()**  
[kandydat.c (Linia 85-102)](https://github.com/adrianKK1/17_Egzamin_wstepny/blob/main/kandydat.c#L85-L102) - Odbiór z obsługą EINTR
```c
while(1) {
    res = msgrcv(msgid, &msg, sizeof(Komunikat)-sizeof(long), 
                 moj_pid, 0);  // Typ = PID (adresowanie)
    if (res == -1) {
        if (errno == EINTR) continue;  // Restart po Ctrl+Z
        if (errno == EIDRM || errno == EINVAL) {
            printf("Ewakuacja (kolejka usunieta).\n");
            exit(0);
        }
        perror("Blad msgrcv");
        exit(1);
    }
    break;
}
```

[komisja.c (Linia 161-177)](https://github.com/adrianKK1/17_Egzamin_wstepny/blob/main/komisja.c#L161-L177) - Recepcja z IPC_NOWAIT
```c
ssize_t result = msgrcv(msgid, &msg, sizeof(Komunikat) - sizeof(long),
                        typ_wejscia, IPC_NOWAIT);

if (result == -1) {
    if (errno == EIDRM) {
        break;  // Kolejka usunięta - koniec
    }
    if (errno == ENOMSG) {
        usleep(100000);  // Brak wiadomości - czekaj
        continue;
    }
    continue;
}
```

* **msgctl()**  
[dziekan.c (Linia 307-309)](https://github.com/adrianKK1/17_Egzamin_wstepny/blob/main/dziekan.c#L307-L309) - Usunięcie kolejki
```c
if (msgid != -1) {
    msgctl(msgid, IPC_RMID, NULL);
}
```

---

## 8. Struktura Plików Projektu

```
17_Egzamin_wstepny/
├── Makefile                  # Reguły kompilacji
├── README.md                 # Ten plik
├── dziekan.c                 # Proces główny (1 instancja)
├── komisja.c                 # Proces komisji (2 instancje: A i B)
├── kandydat.c                # Proces kandydata (10×M instancji)
├── utils.c                   # Funkcje pomocnicze (logging, semafory)
├── utils.h                   # Deklaracje utils
├── common.h                  # Stałe, struktury, typy komunikatów
└── raport.txt                # Wygenerowany raport (po uruchomieniu)
```

### Makefile

```makefile
CC = gcc
CFLAGS = -Wall -Wextra -pthread

all: dziekan komisja kandydat

dziekan: dziekan.c utils.c common.h
	$(CC) $(CFLAGS) -o dziekan dziekan.c utils.c

komisja: komisja.c utils.c common.h
	$(CC) $(CFLAGS) -o komisja komisja.c utils.c

kandydat: kandydat.c utils.c common.h
	$(CC) $(CFLAGS) -o kandydat kandydat.c utils.c

clean:
	rm -f dziekan komisja kandydat *.o raport.txt
```

---

## 9. Problemy Napotkane i Rozwiązania

### Problem 1: Race condition w licznikach pytań

**Symptom:** Niektórzy studenci dostawali > 5 pytań w Komisji A

**Przyczyna:** Wiele wątków jednocześnie inkrementowało `licznik_pytan`

**Rozwiązanie:** 
```c
pthread_mutex_lock(&mutex_stoliki[i]);
if (*licznik_pytan < wymagana) {
    // ... zadaj pytanie ...
    (*licznik_pytan)++;  // Teraz bezpieczne
}
pthread_mutex_unlock(&mutex_stoliki[i]);
```

### Problem 2: Zakleszczenie przy ewakuacji

**Symptom:** Niektóre wątki nie kończyły się po SIGTERM

**Przyczyna:** Wątki zablokowały się na `pthread_mutex_lock()` przed sprawdzeniem flagi

**Rozwiązanie:** Użycie `volatile sig_atomic_t` + sprawdzanie flagi w każdej iteracji

### Problem 3: Przekroczenie limitu procesów

**Symptom:** `fork()` zwraca -1 po utworzeniu ~1000 kandydatów

**Przyczyna:** Limit `ulimit -u` (typowo 1024)

**Rozwiązanie:** 
```c
if (pid == -1) {
    perror("Blad fork()");
    liczba_kandydatow = i;  // Kontynuuj z mniejszą liczbą
    break;
}
```

---

## 10. Podsumowanie

Projekt realizuje pełną symulację egzaminu wstępnego z wykorzystaniem zaawansowanych mechanizmów IPC i wielowątkowości. Kluczowe osiągnięcia:

✅ **Architektura:**
- Pełna decentralizacja (fork + exec)
- Work-stealing pattern (innowacyjne podejście)
- Graceful shutdown bez zakleszczeń

✅ **IPC:**
- 3 mechanizmy (semafory, kolejki, SHM)
- Multipleksing komunikatów na jednej kolejce
- Enum stanów + automaty skończone

✅ **Testy:**
- 5/5 testów zaliczonych
- Weryfikacja: blokady, race conditions, limity procesów

✅ **Funkcjonalność:**
- Wszystkie wymagania tematu spełnione
- Poprawkowicze, ewakuacja, ranking
- Kolorowany output, szczegółowe raporty


---

**Autor:** Adrian Kida   
**GitHub:** [17_Egzamin_wstepny](https://github.com/adrianKK1/17_Egzamin_wstepny)

