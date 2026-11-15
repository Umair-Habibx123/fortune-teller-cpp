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
  "Ayesha Khan\n2023-02-30\n2030-01-01\n2002-05-14\n5\n6\n4\n0\n"
  ""
  "That isn't a real date"
  "That date is in the future"
  "Taurus - The Bull"
  "age 24"
  "A steady week for Taurus"
  "Powered by FreeHoroscopeAPI"
  "14    21    15    18"
  "Taurus + Cancer"
  "Goodbye")

# 2. Returning person: welcomed back; today's horoscope isn't saved and we're
#    offline, so it explains why instead of inventing one; the people book lists them.
run_session(returning
  "\n2\n7\n1\n4\n0\n"
  ""
  "Welcome back, Ayesha Khan (Taurus)"
  "No horoscope available right now: no internet connection"
  "Ayesha Khan"
  "2002-05-14  Taurus")

# 3. Command-line shortcuts.
run_session(cli_sign "" "--sign taurus" "Taurus - weekly horoscope (2026-11-09)" "A steady week for Taurus")
run_session(cli_birthday "" "--birthday 1999-12-25" "Capricorn (The Goat)" "Not available")
if(NOT last_code EQUAL 1)
  message(SEND_ERROR "[cli_birthday] expected exit code 1 when no reading is available, got ${last_code}")
endif()
run_session(cli_bad_sign "" "--sign ophiuchus" "Unknown sign: ophiuchus")
