<h1 align="center">Fortune Teller: A First-Semester Project, Grown Up</h1>

<p align="center">
  <em>My first-semester C program, rebuilt in modern C++17 with live horoscopes, tarot, planetary positions and a daily Panchang.</em>
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
original writing, and adds **live data that updates by itself**: horoscopes,
tarot, where the planets are, sunrise and moonrise, and the Vedic Panchang.

---

## Features

### Live (from [FreeHoroscopeAPI](https://freehoroscopeapi.com))

- **Horoscopes**: this week's, today's and this month's reading for your sign.
  The first time you open the app in a new day, week or month it fetches the
  latest reading and saves it.
- **Tarot**: a card of the day (the same card all day), a past-present-future
  spread, and the full 78-card deck to browse by suit or search by name, with
  upright and reversed meanings. The deck is downloaded once and then works offline.
- **Planets right now**: the sign and degree of the Sun, Moon, planets and the
  lunar nodes, with retrograde markers. Tropical (Western) or sidereal (Vedic,
  with nakshatra and pada).
- **Your birth planets**: the same chart for noon on your birthday.
- **Sun and Moon** for your city: dawn, sunrise, solar noon, sunset, dusk, day
  length, golden hour, moonrise, moonset, Moon phase and Rahu Kaal.
- **Panchang**: the day's tithi, nakshatra, yoga and karana, with when each changes.
- **Works offline.** Everything downloaded is saved. With no internet, saved data
  is shown and marked with the date it's from. If nothing is saved, it says so.
  It never makes one up. Failed calls are retried a couple of times with a short
  pause first.

### Classic (from the 2018 original)

- **Your sign** from your birthday, with its ASCII drawing, element, ruling planet,
  a countdown to your next birthday and your Chinese zodiac animal.
- **About you**: personality notes, picked from your birthday the same way the
  original did.
- **Fortune cookie**: a random fortune from the original collection.
- **Lucky numbers**: the original formula.
- **Love compatibility** for all 78 sign pairs, plus how your elements mix.
- **People book**: everyone who got a reading on this computer, with birthday
  search and delete (the old "administrator tools").
- **Remembers you**: next time it says "Welcome back" and goes straight to your sign.

### Settings

- **Location**: pick from a list of 35 cities, or type latitude and longitude.
  Only needed for Sun and Moon and the Panchang. Nothing is detected automatically.
- **Zodiac system**: tropical or sidereal.
- **Clock**: 24-hour or 12-hour.

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
fortune --sign leo --daily         # today's instead (--monthly for this month's)
fortune --birthday 2002-05-14      # find the sign for a birthday + its weekly horoscope
fortune --tarot 3                  # draw 3 tarot cards (1-10)
fortune --planets                  # where the planets are right now
fortune --city london              # sunrise, sunset and the Moon for a listed city
fortune --sun                      # the same for the city saved in Settings
fortune --offline                  # saved data only, no internet
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

Readings, settings and the people book stay on your computer, outside the repo:

| OS | Folder |
|---|---|
| Windows | `%APPDATA%\FortuneTeller` |
| macOS / Linux | `~/.local/share/fortune-teller` (or `$XDG_DATA_HOME/fortune-teller`) |

Set `FORTUNE_TELLER_HOME` to use a different folder.

What's sent over the internet: a zodiac sign name (horoscopes), the zodiac system
(planets), and, only if you choose a location, its coordinates (Sun and Moon,
Panchang). For your birth planets, your birth date is sent. Names and the people
book never leave your computer.

---

## Project layout

```text
src/
  main.cpp          command-line options, starts the app
  App.cpp           the interactive menus
  Horoscope.cpp     live readings + the daily/weekly/monthly cache
  Tarot.cpp         the deck, draws, card of the day, search
  Sky.cpp           planets, Sun and Moon, Panchang, cities and settings
  Api.cpp           API calls with retries + the JSON cache
  Http.cpp          HTTPS: WinHTTP on Windows, libcurl elsewhere
  Json.cpp          a small JSON parser
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
  unit_tests.cpp    dates, signs, pairs, JSON, caches, tarot, planets, settings
  smoke_test.cmake  runs the real program with scripted input and saved API answers (offline)
```

## Tests

```bash
ctest --test-dir build --output-on-failure
```

The tests never use the internet. CI runs them on Windows, macOS and Linux.

---

## Credits

Horoscopes, tarot, planetary positions, Sun and Moon times and the Panchang are
[Powered by FreeHoroscopeAPI](https://freehoroscopeapi.com).

For fun only. Please don't make life decisions based on a console app.

Released under the [MIT License](LICENSE).
