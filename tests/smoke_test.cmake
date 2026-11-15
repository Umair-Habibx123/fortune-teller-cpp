# End-to-end smoke test: drives the real program with scripted keyboard input
# and checks what it prints. Runs without internet (FORTUNE_OFFLINE=1) and with
# its own data folder, so it never touches your saved readings.
#
# Expects -DAPP=<path to the fortune executable> and -DWORK=<scratch folder>.

file(REMOVE_RECURSE "${WORK}")
file(MAKE_DIRECTORY "${WORK}/home/cache")

set(ENV{FORTUNE_TELLER_HOME} "${WORK}/home")
set(ENV{FORTUNE_OFFLINE} "1")
set(ENV{FORTUNE_TODAY} "2026-11-15")
set(ENV{FORTUNE_SEED} "7")
set(ENV{NO_COLOR} "1")

# A reading saved earlier this week, as if it had been downloaded on Monday.
file(WRITE "${WORK}/home/cache/weekly-taurus.txt"
  "2026-11-09\n2026-11-09\nA steady week for Taurus: finish what you started.\n")

# Hand-made API answers in the cache format (first line = date fetched, then the JSON).
file(WRITE "${WORK}/home/cache/tarot-deck.json" "2026-11-01\n{\"nhits\":3,\"cards\":[
{\"type\":\"major\",\"name\":\"The Star\",\"name_short\":\"ar17\",\"value_int\":17,\"meaning_up\":\"Hope and renewal.\",\"meaning_rev\":\"Doubt.\",\"desc\":\"A woman kneels by a pool.\"},
{\"type\":\"minor\",\"name\":\"Ace of Cups\",\"name_short\":\"cuac\",\"value_int\":1,\"suit\":\"cups\",\"meaning_up\":\"New feelings.\",\"meaning_rev\":\"Held back.\",\"desc\":\"A cup overflowing.\"},
{\"type\":\"minor\",\"name\":\"Two of Wands\",\"name_short\":\"wa02\",\"value_int\":2,\"suit\":\"wands\",\"meaning_up\":\"Planning ahead.\",\"meaning_rev\":\"Fear of change.\",\"desc\":\"A man holds a globe.\"}]}")
file(WRITE "${WORK}/home/cache/planets-now-tropical.json" "2026-11-15\n{\"datetime\":\"2026-11-15T09:00:00Z\",\"bodies\":{
\"sun\":{\"sign\":\"Scorpio\",\"degrees_in_sign\":23.25,\"retrograde\":false},
\"mercury\":{\"sign\":\"Sagittarius\",\"degrees_in_sign\":4.5,\"retrograde\":true}}}")
file(WRITE "${WORK}/home/cache/times-2026-11-15-51.5074_-0.1278.json" "2026-11-15\n{\"timezone\":\"Europe/London\",
\"sun\":{\"events\":{\"dawn\":\"2026-11-15T06:42:00+00:00\",\"sunrise\":\"2026-11-15T07:19:00+00:00\",\"noon\":\"2026-11-15T11:45:00+00:00\",\"sunset\":\"2026-11-15T16:11:00+00:00\",\"dusk\":\"2026-11-15T16:48:00+00:00\"},
\"daylight\":{\"duration_seconds\":31920,\"change_from_yesterday_seconds\":-150},
\"photography\":{\"golden_hour\":{\"morning\":{\"start\":\"2026-11-15T07:19:00+00:00\",\"end\":\"2026-11-15T08:05:00+00:00\"},\"evening\":{\"start\":\"2026-11-15T15:25:00+00:00\",\"end\":\"2026-11-15T16:11:00+00:00\"}}}},
\"moon\":{\"moonrise\":\"2026-11-15T10:02:00+00:00\",\"moonset\":\"2026-11-15T18:30:00+00:00\",\"phase_description\":\"Waxing Crescent\",\"details\":{\"illumination_pct\":21.4}},
\"rahukaalam\":{\"start\":\"2026-11-15T14:52:00+00:00\",\"end\":\"2026-11-15T16:11:00+00:00\"}}")
file(WRITE "${WORK}/home/cache/panchang-2026-11-15-51.5074_-0.1278.json" "2026-11-15\n{\"weekday\":\"Sunday\",\"timezone\":\"Europe/London\",
\"tithi\":{\"at_start\":{\"number\":5,\"name\":\"Panchami\",\"paksha\":\"Shukla\"},\"transitions\":[{\"datetime\":\"2026-11-15T20:10:00+00:00\",\"to\":{\"name\":\"Shashthi\"}}]},
\"nakshatra\":{\"at_start\":{\"name\":\"Uttara Ashadha\"},\"transitions\":[]},
\"yoga\":{\"at_start\":{\"name\":\"Ganda\"}},\"karana\":{\"at_start\":{\"name\":\"Bava\"}}}")

# run_session(<name> <input text> <args> <expected text>...)
function(run_session name input args)
  file(WRITE "${WORK}/${name}.in" "${input}")
  separate_arguments(arg_list UNIX_COMMAND "${args}")
  execute_process(
    COMMAND "${APP}" ${arg_list}
    WORKING_DIRECTORY "${WORK}"
    INPUT_FILE "${WORK}/${name}.in"
    OUTPUT_VARIABLE out
    ERROR_VARIABLE out
    RESULT_VARIABLE code
    TIMEOUT 30)
  file(WRITE "${WORK}/${name}.out" "${out}")
  foreach(expected IN LISTS ARGN)
    string(FIND "${out}" "${expected}" at)
    if(at EQUAL -1)
      message(SEND_ERROR "[${name}] expected to see: ${expected}\n--- output ---\n${out}")
    else()
      message(STATUS "[${name}] ok: ${expected}")
    endif()
  endforeach()
  set(last_code ${code} PARENT_SCOPE)
endfunction()

# 1. New person: bad dates are rejected, the sign is found, the saved weekly
#    reading is shown, then lucky numbers, compatibility and exit.
run_session(new_person
  "Ayesha Khan\n2023-02-30\n2030-01-01\n2002-05-14\n13\n14\n4\n0\n"
  ""
  "That isn't a real date"
  "That date is in the future"
  "Taurus - The Bull"
  "age 24"
  "180 days to your birthday"
  "Chinese zodiac: Horse"
  "A steady week for Taurus"
  "Powered by FreeHoroscopeAPI"
  "14    21    15    18"
  "Taurus + Cancer"
  "Earth and Water"
  "Goodbye")

# 2. Returning person: welcomed back; today's horoscope isn't saved and we're
#    offline, so it explains why instead of inventing one; the people book lists them.
run_session(returning
  "\n2\n16\n1\n4\n0\n"
  ""
  "Welcome back, Ayesha Khan (Taurus)"
  "No horoscope available right now: no internet connection"
  "Ayesha Khan"
  "2002-05-14  Taurus")

# 3. Tarot, planets and the city-based screens, all from the saved fixtures.
#    Sun & Moon first says a city is needed; then London is picked in Settings
#    (city 20), the clock switched to 12-hour, and the screens work.
run_session(sky_and_tarot
  "\n4\n5\n6\n2\n7\n9\n15\n1\n20\n3\n4\n9\n10\n3\n0\n"
  ""
  "Your tarot card of the day"
  "Past:"
  "Future:"
  "Ace of Cups"
  "Upright"
  "Planning ahead."
  "Mercury"
  "retrograde"
  "Choose your city in Settings first"
  "Saved: London, UK"
  "Sunrise         7:19 AM"
  "Golden hour     7:19 AM-8:05 AM and 3:25 PM-4:11 PM"
  "Waxing Crescent  (21% lit)"
  "Rahu Kaal       2:52 PM-4:11 PM"
  "Panchami (Shukla paksha)"
  "-> Shashthi from 8:10 PM"
  "Uttara Ashadha"
  "This month's horoscope"
  "No horoscope available right now")

# 4. Command-line shortcuts.
run_session(cli_tarot "" "--tarot 2" "Powered by FreeHoroscopeAPI")
run_session(cli_sun "" "--city london" "London, UK" "Sunset      4:11 PM" "Waxing Crescent")
run_session(cli_bad_city "" "--city atlantis" "Unknown city: atlantis")
run_session(cli_planets "" "--planets" "Scorpio 23 deg" "Sagittarius 4 deg  retrograde")
run_session(cli_sign "" "--sign taurus" "Taurus - weekly horoscope (2026-11-09)" "A steady week for Taurus")
run_session(cli_birthday "" "--birthday 1999-12-25" "Capricorn (The Goat)" "Not available")
if(NOT last_code EQUAL 1)
  message(SEND_ERROR "[cli_birthday] expected exit code 1 when no reading is available, got ${last_code}")
endif()
run_session(cli_bad_sign "" "--sign ophiuchus" "Unknown sign: ophiuchus")
