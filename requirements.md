# Zadanie 1 – Edytor wielokątów i wielokątów krzywoliniowych

Liczba punktów: 17 (projekt) + 8 (lab)

## Podstawowa specyfikacja

### 1. Obsługa wielokątów

- Możliwość umieszczenia na scenie dowolnej liczby wielokątów (poprzez scenę początkową).
- Wielokąty mogą posiadać samoprzecięcia.
- Możliwość edycji dowolnego wielokąta.
- Możliwość przesuwania całego wielokąta.

Podczas edycji wielokąta należy umożliwić:

- przesuwanie wierzchołka,
- przesuwanie punktu kontrolnego segmentu Béziera,
- usuwanie wierzchołka,
- dodawanie wierzchołka w środku wybranej krawędzi.

Uwaga! Nie jest wymagane tworzenie wielokątów i usuwanie ich przez użytkownika – mogą być one umieszczone na scenie początkowej.

### 2. Ograniczenia (relacje) geometryczne

Dla wybranej krawędzi lub pary wybranych krawędzi (również należących do różnych wielokątów) należy umożliwić definiowanie ograniczeń geometrycznych.

Wymagane rodzaje ograniczeń:

- Zada na długość krawędzi – użytkownik podaje długość krawędzi w oknie edycyjnym. Jako wartość domyślna należy przyjąć jej aktualną długość.
- Równoległość dwóch krawędzi – dla osób o nazwiskach od A do K.
- Prostopadłość dwóch krawędzi – dla osób o nazwiskach od L do Z.

Dodatkowo:

- dla jednej krawędzi może być zdefiniowanych kilka różnych ograniczeń;
- dodanie wierzchołka na krawędzi lub usunięcie wierzchołka powinno powodować usunięcie ograniczeń związanych z tą krawędzią oraz odpowiednią aktualizację relacji krawędzi sąsiednich;
- aktywne ograniczenia powinny być widoczne na scenie, np. w postaci odpowiednich ikon umieszczonych w pobliżu środka krawędzi;
- należy umożliwić usuwanie wcześniej zdefiniowanych ograniczeń.

### 3. Segmenty Béziera trzeciego stopnia

Dla każdej krawędzi należy umożliwić przełączanie pomiędzy zwykłym odcinkiem a segmentem Béziera trzeciego stopnia. Po przełączeniu krawędź nie jest już odcinkiem, lecz krzywą.

Po włączeniu trybu Béziera:

- należy wyświetlić wielobok Béziera, np. za pomocą linii przerywanej;
- należy wyświetlić dwa dodatkowe punkty kontrolne;
- punkty kontrolne powinny być możliwe do przesuwania za pomocą myszy.

Jeżeli wierzchołek sąsiaduje z segmentem Béziera, należy umożliwić określenie wymaganej ciągłości w tym wierzchołku:

- G0 – ciągłość położenia,
- G1 – ciągłość kierunku (jednostkowego wektora stycznego),
- C1 – ciągłość wektora stycznego.

Przełączenie krawędzi z odcinka na segment Béziera powinno automatycznie ustawić domyślną klasę ciągłości w sąsiednich wierzchołkach, chyba że ciągłość została wcześniej jawnie określona przez użytkownika:

- G1 – osoby o nazwiskach od A do K,
- C1 – osoby o nazwiskach od L do Z.

Podczas przesuwania wierzchołków i punktów kontrolnych należy uwzględniać ustawioną klasę ciągłości. Oznacza to, że zmiana położenia jednego elementu może wymuszać odpowiednią zmianę położenia innych elementów.

### 4. Łuki okręgów

Dla każdej krawędzi należy również umożliwić przełączanie pomiędzy zwykłym odcinkiem a łukiem okręgu.

Jeżeli wierzchołek sąsiaduje z łukiem, należy umożliwić określenie wymaganej ciągłości:

- G0 – ciągłość położenia,
- G1 – ciągłość kierunku (jednostkowego wektora stycznego).

Dodatkowe wymagania:

- tylko jeden koniec łuku może mieć ustawioną ciągłość G1;
- jeżeli oba końce łuku mają ciągłość G0, łuk powinien być rysowany z promieniem równym połowie długości krawędzi.

### 5. Algorytmy rysowania

- Rysowanie odcinków – należy umożliwić wybór pomiędzy algorytmem bibliotecznym a własną implementacją algorytmu Bresenhama. Wybór powinien być realizowany za pomocą przycisków typu radio button.
- Rysowanie segmentu Béziera – należy zastosować algorytm przyrostowy: przejście do bazy potęgowej, a następnie szybki iteracyjny algorytm wyznaczania kolejnych punktów krzywej.

### 6. Obsługa myszy i edycja

Definiowanie nowych wielokątów oraz ich późniejsza edycja powinny być możliwie intuicyjne.

- przesuwanie elementów odbywa się przy użyciu lewego przycisku myszy;
- przesuwany wierzchołek lub punkt kontrolny powinien zawsze podążać za kursorem myszy;
- przesuwanie punktu kontrolnego nie powinno, poza wyjątkowymi przypadkami wynikającymi z ustawionych ograniczeń, powodować przesuwania wierzchołków danego segmentu;
- przesunięcie punktu kontrolnego może natomiast powodować przesunięcie innych wierzchołków lub punktów kontrolnych, jeżeli jest to wymagane przez zdefiniowane ograniczenia i relacje.

### 7. Model danych i architektura

W ramach realizacji zadania należy położyć szczególny nacisk na:

- algorytmy,
- model danych,
- architekturę programu,
- implementację mechanizmu ograniczeń i relacji.

Wygląd interfejsu użytkownika (UI/UX) ma znaczenie drugorzędne. Nie należy poświęcać nadmiernej ilości czasu na dopracowywanie warstwy wizualnej kosztem implementacji wymaganych algorytmów i mechanizmów geometrycznych.

### 8. Scena początkowa

Po uruchomieniu programu powinna automatycznie pojawić się predefiniowana scena demonstracyjna.

Scena powinna zawierać:

- co najmniej dwa wielokąty,
- zdefiniowane ograniczenia geometryczne,
- co najmniej jedną krawędź zamienioną na segment Béziera.

Zwiększenie liczby wielokątów w scenie początkowej (za pomocą kodu) – powinno być bardzo proste i czytelne.

Scena początkowa powinna umożliwiać bezpośrednie zaprezentowanie najważniejszych funkcjonalności programu.

### 9. Dokumentacja

Do programu należy przygotować prostą dokumentację, najlepiej dostępną bezpośrednio z poziomu aplikacji.

Dokumentacja powinna zawierać co najmniej:

- instrukcję obsługi, w szczególności opis obsługi myszy oraz „klawiszologię”;
- opis przyjętych założeń;
- opis modelu danych;
- opis zaimplementowanych algorytmów, w szczególności algorytmu obsługi ograniczeń (relacji).

### 10. Termin oddania

Termin oddania zadania: 27–28 października.

W trakcie tych zajęć odbędzie się część laboratoryjna.


### 11. Laboratoria oceniane

Co dodać:
 - Antialiasing metodą 'multisampling';
