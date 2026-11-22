# Magistrala CAN

Software'owa symulacja magistrali standardu CAN2.0A napisana w `C++23`.
Nie zostały użyte żadne zewnętrzne biblioteki. Pozwala na modelowanie
modułów oraz przypisywanie im wiadomości z określoną częstotliwością 
oraz terminem dostarczenia.

## Budowanie
Wymagania kompilator C++ wspierający standard C++23 (np. `gcc 14`)
Opcjonalnie `CMake`, interpreter `Python'a` oraz `Matplotlib` do generowania wykresów

```bash
cmake -B build
cmake --build ./build/
```

Bez programu `cmake` można też 
```bash
mkdir build
g++ -std=c++23 ./src/*.cpp -o ./build/can_bus
```

Generowanie wykresu
```bash
python3 plot.py
```

## Funkcjonalności
* Model linii `CAN_H` i `CAN_L` oraz ich stanów.
* Ramki zgodne ze standardem CAN2.0A: 11-bitowe ID, Bit Stuffing, pola 
  związane z narzutem protokołu np. `SOF`, `CRC`, `ACK` i `EOF`.
* Arbitraż - przegrywająca ramka nie jest odrzuczana, a moduł wraca do stanu oczekiwania na linię.
* Kolejkowanie wiadomości w poszczególnych modułach według różnych strategii:
  * `FIFO` - wiadomość o wcześniejszym czasie stworzenia ma pierwszeństwo
  * `IdPriority` - wiadomość o niższym ID ma pierwszeństwo nawet jeżeli inna wiadomość jest już w kolejce
  * `DeadlinePriority` - wiadomość o wczeniejszym terminie dostarczenia ma pierwszeństwo
* Symulacja przesyłu danych w sieci bit po bicie oraz sprawdzenie czy nie występują zduplikowane ID.

## Struktura projektu
* `CanNetwork` pełni jedynie rolę logicznego grupowania modułów oraz bus'u, 
  odpowiada za krok czasowy oraz agregację statystyk, przekazuje `CanBus` do `CanNode`
* `CanNode` sama zarządza swoim stanem, wykonuje arbitraż na podstawie ID oraz sprawdza 
  czy linia jest zajęta, aby lepiej oddać idee protokołu CAN.
* `CanBus` przechowuje informację o aktualnym stanie napięć na linii, ułatwia
  urzytkownikom logikę wired-AND metodą `CanBus::drive_dominant`.
* `CanFrame` przechowuje tablicę bitów z których składa się ramka.

## Dla ambitnych
* **Sprawdzenie czy identyfikatory są optymalne** \
  W podanej tabelce `ID CAN` nie były optymalne, ponieważ np. w module `Silnik`
  ciśnienie o niższym terminie dostarczenia niż temperatura ma wyższe ID.
  Można to naprawić stosując inną strategię kolejkowania np. `DeadlinePriority` w module.
  Wiadomość `Pozycja GPS` w module `Nawigajca` ma wyższe ID niż `Napięcie akumulatora` w module 
  `Zasilanie`, mimo że ma ona termin dostarczenia w przeciwieństwie do drugiej wiadomości.
* **Wizualizacja obciążenia magistrali w czasie** \ 
  klasa `CanNetwork` zbiera informacje o obiciążeniu
  magistrali w odstepie czasowym ustawianym w przekazywanym w konstruktorze,
  Dane są zapisywane do pliku `bus_load.csv` jest również generowany wykres
  skryptem w Python'ie `plot.py` o nazwie `bus_load.png`