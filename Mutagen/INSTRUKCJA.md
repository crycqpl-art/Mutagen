# MUTAGEN – instrukcja

Mutagen to efekt VST3 (i AU na Macu) do Abletona. Wrzucasz go na ścieżkę z melodią, samplem z innego utworu albo progresją akordów, a on przerabia dźwięk w rytmie utworu.

## Jak to działa (od lewej do prawej)

1. **SEQUENCER** (16 kroków, w stylu Gross Beat). Każdy krok mówi, co ma się dziać z dźwiękiem w danym momencie taktu:
   - **PASS** – oryginał bez zmian
   - **STUT** – stutter / ratchet (krótkie powtórzenia)
   - **REV** – poprzedni krok odtworzony od tyłu
   - **HALF** – pół prędkości (oktawa niżej, efekt taśmy)
   - **OCT+** – podwójna prędkość (oktawa wyżej)
   - **FRZ** – zamrożenie dźwięku (trzyma ostatni fragment)
   - **STOP** – tape stop (zwalnianie taśmy)
   - **GATE** – cisza
   - **SCAT** – skok do losowego wcześniejszego fragmentu (przetasowanie)

   Klik = następny tryb, prawy klik = lista, Shift+klik = PASS, przeciąganie = malowanie tym samym trybem.
   **RATE** ustawia długość kroku (1/4, 1/8, 1/16, 1/32). Sekwencer synchronizuje się z tempem i transportem Abletona (gdy transport stoi, chodzi na wewnętrznym zegarze).

2. **CHORD CLOUD** – granulator, który tnie dźwięk na drobinki i odtwarza je w akordzie (Major, Minor, Maj7, Add9 Dream, Dark b9…). Z melodii robi pady, chóry i chmury.
   Amount = ile chmury, Grain Size = długość drobinek, Density = ile na sekundę, Spread = jak daleko w przeszłość sięgają, Octave = transpozycja.

3. **SPECTRAL SMEAR** – rozmywa widmo w czasie (Blur) i zamienia wszystko w miękką, płynącą teksturę. **FREEZE** zatrzymuje aktualne brzmienie jako nieskończony dron.

4. **FILTER** – low-pass / band-pass / high-pass z rezonansem.

5. **SPACE + OUTPUT** – pogłos, dry/wet i głośność wyjścia.

**DICE PATTERN** losuje wzór kroków, **DICE ALL** losuje wszystko. **Presets** to 10 gotowych brzmień na start.

Wszystkie gałki da się automatyzować w Abletonie i mapować na kontroler MIDI.

## Instalacja

### Windows
1. Rozpakuj pobrany plik `Mutagen-Windows.zip`.
2. Skopiuj folder **Mutagen.vst3** do `C:\Program Files\Common Files\VST3\`.
3. Ableton: Options → Preferences (Settings) → Plug-Ins → włącz **Use VST3 Plug-In System Folders** → **Rescan**.
4. Mutagen pojawi się w przeglądarce w Plug-Ins → Kamil Potebski → Mutagen.

### Mac
1. Rozpakuj `Mutagen-macOS.zip`.
2. Skopiuj **Mutagen.vst3** do `~/Library/Audio/Plug-Ins/VST3/` (i opcjonalnie **Mutagen.component** do `~/Library/Audio/Plug-Ins/Components/`).
3. Wtyczka nie jest podpisana certyfikatem Apple, więc raz trzeba zdjąć blokadę. W Terminalu:
   ```
   xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/Mutagen.vst3
   xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/Components/Mutagen.component
   ```
4. Ableton: Settings → Plug-Ins → włącz VST3 / Audio Units → **Rescan**.

## Budowanie (gdy chcesz zmienić kod)

Projekt buduje się sam na GitHubie (zakładka **Actions**) po każdej zmianie. Lokalnie: zainstaluj CMake i Visual Studio 2022 (Windows) lub Xcode (Mac), potem:

```
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Gotowa wtyczka będzie w `build/Mutagen_artefacts/Release/VST3/`.

Uwaga licencyjna: wtyczka używa frameworka JUCE. Do użytku własnego nic nie musisz robić. Jeśli kiedyś zechcesz ją sprzedawać, sprawdź licencję JUCE (juce.com).
