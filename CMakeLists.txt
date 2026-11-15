cmake_minimum_required(VERSION 3.15)
project(fortune-teller VERSION 2.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Everything except main() lives in a library so the tests can use it too.
add_library(fortune_core STATIC
    src/Api.cpp
    src/App.cpp
    src/Console.cpp
    src/Date.cpp
    src/Horoscope.cpp
    src/Http.cpp
    src/Json.cpp
    src/LegacyData.cpp
    src/Readings.cpp
    src/Sky.cpp
    src/Storage.cpp
    src/Tarot.cpp
    src/Zodiac.cpp
)
target_include_directories(fortune_core PUBLIC src)

# HTTPS: Windows has WinHTTP built in; macOS and Linux use libcurl.
if(WIN32)
    target_link_libraries(fortune_core PUBLIC winhttp)
else()
    find_package(CURL REQUIRED)
    target_link_libraries(fortune_core PUBLIC CURL::libcurl)
endif()

add_executable(fortune src/main.cpp)
target_link_libraries(fortune PRIVATE fortune_core)

foreach(target fortune_core fortune)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /utf-8)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endforeach()

# MinGW: ship a self-contained .exe (no missing libstdc++/libgcc DLLs).
if(MINGW)
    target_link_options(fortune PRIVATE -static)
endif()

enable_testing()

add_executable(unit_tests tests/unit_tests.cpp)
target_link_libraries(unit_tests PRIVATE fortune_core)
if(MINGW)
    target_link_options(unit_tests PRIVATE -static)
endif()
add_test(NAME unit COMMAND unit_tests)

# End-to-end: drives the real program with scripted input, without internet.
add_test(NAME smoke
    COMMAND ${CMAKE_COMMAND}
        -DAPP=$<TARGET_FILE:fortune>
        -DWORK=${CMAKE_CURRENT_BINARY_DIR}/smoke-work
        -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/smoke_test.cmake)
