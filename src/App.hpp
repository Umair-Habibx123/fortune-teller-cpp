#pragma once
#include "Horoscope.hpp"
#include "Storage.hpp"

// The interactive program: who are you -> your sign -> the menu.
class App {
public:
    App();
    int run();

private:
    bool chooseProfile();            // false when input ended
    bool enterPerson(const std::string& heading);  // ask name + birthday; false when input ended
    bool home();                     // false = exit
    void showSign() const;
    void showHoroscope(Period period, bool compact = false);
    void showAbout() const;
    void showFortune() const;
    void showLucky() const;
    bool showCompatibility() const;  // false when input ended
    bool peopleBook();               // false when input ended

    Person me_;
    Sign sign_ = Sign::Aries;
    Date today_;
    HoroscopeService horoscopes_;
    PeopleBook people_;
};
