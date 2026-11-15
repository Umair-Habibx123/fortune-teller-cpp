#pragma once
#include <string>

#include "Horoscope.hpp"
#include "Sky.hpp"
#include "Storage.hpp"
#include "Tarot.hpp"

// The interactive program: who are you -> your sign -> the menu.
class App {
public:
    App();
    int run();

private:
    bool chooseProfile();                            // false when input ended
    bool enterPerson(const std::string& heading);    // ask name + birthday; false when input ended
    bool home();                                     // false = exit
    void showSign() const;

    // Live horoscopes
    void showHoroscope(Period period, bool compact = false);
    // Tarot
    void showCardOfTheDay();
    void showSpread();
    bool browseDeck();                               // false when input ended
    // Sky
    void showPlanets(bool atBirth);
    void showSunMoon();
    void showPanchang();
    bool settingsMenu();                             // false when input ended
    bool chooseLocation();                           // false when input ended
    // Classic readings from the original program
    void showAbout() const;
    void showFortune() const;
    void showLucky() const;
    bool showCompatibility() const;                  // false when input ended
    bool peopleBook();                               // false when input ended

    Person me_;
    Sign sign_ = Sign::Aries;
    Date today_;
    HoroscopeService horoscopes_;
    PeopleBook people_;
    Tarot tarot_;
    Sky sky_;
};
