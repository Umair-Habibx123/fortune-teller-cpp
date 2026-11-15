<h1 align="center">🔮 Fortune Teller: A First-Semester Project, Grown Up</h1>

<p align="center">
  <em>My first-semester C program, rebuilt in modern C++17 with real horoscopes from the internet.</em>
</p>

<p align="center">
  <img alt="Language" src="https://img.shields.io/badge/C%2B%2B-17-00599C?logo=cplusplus&logoColor=white">
  <img alt="Build" src="https://img.shields.io/badge/build-CMake-064F8C?logo=cmake&logoColor=white">
  <img alt="Platform" src="https://img.shields.io/badge/platform-Windows%20%7C%20macOS%20%7C%20Linux-555">
  <a href="https://github.com/Umair-Habibx123/fortune-teller-cpp/actions/workflows/ci.yml"><img alt="CI" src="https://github.com/Umair-Habibx123/fortune-teller-cpp/actions/workflows/ci.yml/badge.svg"></a>
  <a href="LICENSE"><img alt="License: MIT" src="https://img.shields.io/badge/license-MIT-blue"></a>
</p>

---

## The story

This was my **1st-semester project**: a console fortune teller written in C. You
typed your date of birth, it worked out your zodiac sign, drew it in ASCII art and
told you about yourself, your fortune, your lucky numbers and who you're compatible
with. It ran on `gotoxy()`, `system("cls")`, busy-wait loops for delays and a
`main()` that called itself to restart.

The original is kept, untouched, in [`legacy/`](legacy/). The version you build
today lives in [`src/`](src/). It keeps every feature and every line of the
original writing, and adds **real horoscopes that update every week**.

---

## Features

- **Live horoscopes.** This week's and today's horoscope for your sign, downloaded
  from [FreeHoroscopeAPI](https://freehoroscopeapi.com). It updates by itself:
  the first time you open the app in a new week (or a new day, for the daily one)
  it fetches the latest reading and saves it.
- **Works offline.** Saved readings are shown when there's no internet, clearly
  marked with the date they're from. If nothing is saved, it says so. It never
  makes one up.
- **Your sign** from your birthday, with its ASCII drawing, element and ruling planet.
- **About you**: personality notes, picked from your birthday the same way the
  original did.
- **Fortune cookie**: a random fortune from the original collection.
- **Lucky numbers**: the original formula.
- **Love compatibility** for all 78 sign pairs.
- **People book**: everyone who got a reading on this computer, with birthday
  search and delete (the old "administrator tools").
- **Remembers you**: next time it says "Welcome back" and goes straight to your sign.

### What changed from the original

| Original (2018) | Now |
|---|---|
| Windows-only (`windows.h`, `conio.h`, `dos.h`) | Windows, macOS and Linux |
| Date check let through dates like 31 April or 29 Feb 2023 | Real calendar check, including leap years |
| Age worked out from the year 2010 | Age from today's date |
| Some love-compatibility pairs showed the wrong text (e.g. Scorpio + Sagittarius) | Every pair shows its own text, in both orders |
| Fortune picked with `rand() % 100` from 99 fortunes (could read past the end) | Picks only real fortunes |
| `records.txt` written as raw C structs, admin password in the source code | Plain text file in your own app-data folder |
| `main()` called itself to "restart" | A normal menu loop |

A few fortunes I wrote at 18 didn't make it into the new version
(e.g. "You're stupid."). They're still in `legacy/` if you want them.

---

## How to run

You need a C++17 compiler and CMake. On **Linux** you also need the libcurl
development package. Windows uses its built-in WinHTTP, and macOS ships libcurl.

**Windows**: install MinGW-w64 and CMake (one time):

```powershell
winget install -e --id BrechtSanders.WinLibs.POSIX.UCRT
winget install -e --id Kitware.CMake
```

**Linux (Debian/Ubuntu)**:

```bash
sudo apt install g++ cmake libcurl4-openssl-dev
```

**macOS**: `xcode-select --install` and `brew install cmake`.

Then, from the project folder:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
./build/fortune          # Windows: build\fortune.exe
```

### Shortcuts

```bash
fortune --sign leo                 # this week's horoscope for Leo
fortune --sign leo --daily         # today's instead
fortune --birthday 2002-05-14      # find the sign for a birthday + its weekly horoscope
fortune --offline                  # saved readings only, no internet
fortune --help
```

### The original version

The original runs on Windows only. Compile it as C:

```powershell
gcc -x c -std=gnu89 -w legacy/C_PROJEC.CPP -o legacy-fortune.exe
.\legacy-fortune.exe
```

---

## Where your data is saved

Readings and the people book stay on your computer, outside the repo:

| OS | Folder |
|---|---|
| Windows | `%APPDATA%\FortuneTeller` |
| macOS / Linux | `~/.local/share/fortune-teller` (or `$XDG_DATA_HOME/fortune-teller`) |

Set `FORTUNE_TELLER_HOME` to use a different folder. The only thing sent over the
internet is the name of a zodiac sign, to fetch its horoscope.

---

## Project layout

```text
src/
  main.cpp          command-line options, starts the app
  App.cpp           the interactive menus
  Horoscope.cpp     live readings + the weekly/daily cache
  Http.cpp          HTTPS: WinHTTP on Windows, libcurl elsewhere
  Json.cpp          reads the API's JSON
  Zodiac.cpp        signs, date ranges, compatibility pairs
  Date.cpp          calendar maths, ages, ISO weeks
  Readings.cpp      about you, lucky numbers, fortunes, compatibility
  Storage.cpp       data folder and the people book
  Console.cpp       colors, wrapping, prompts
  LegacyData.cpp    GENERATED: the original texts (see tools/)
legacy/             the original 2018 program, unchanged
tools/
  extract_legacy.py rebuilds src/LegacyData.cpp from legacy/
tests/
  unit_tests.cpp    dates, signs, pairs, JSON, cache, people book
  smoke_test.cmake  runs the real program with scripted input (offline)
```

## Tests

```bash
ctest --test-dir build --output-on-failure
```

The tests never use the internet. CI runs them on Windows, macOS and Linux.

---

## Credits

Horoscopes are [Powered by FreeHoroscopeAPI](https://freehoroscopeapi.com).

For fun only. Please don't make life decisions based on a console app. 🙂

Released under the [MIT License](LICENSE).
