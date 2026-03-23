#include "input.hpp"
#include <iostream>
#include <limits>
#ifdef _WIN32
#include <windows.h>
#endif

#define RESET   "\033[0m"
#define BOLD    "\033[1m"
#define CYAN    "\033[36m"
#define RED     "\033[31m"
#define YELLOW  "\033[33m"

static int getTermWidth() {
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    return csbi.srWindow.Right - csbi.srWindow.Left + 1;
#else
    return 80;
#endif
}

static std::string centerPad(const std::string& text) {
    int termWidth = getTermWidth();
    int pad = (termWidth - (int)text.size()) / 2;
    if (pad < 0) pad = 0;
    return std::string(pad, ' ') + text;
}

// ─────────────────────────────────────────────────
std::string inputLine(const std::string& prompt, bool optional,
                      std::function<void()> reprint) {
    std::string err = "";
    while (true) {
        if (!err.empty()) {
            system("cls");
            if (reprint) reprint();
            std::cout << RED << BOLD << centerPad(err) << "\n\n" << RESET;
        }
        std::cout << CYAN << BOLD << prompt << RESET;
        std::string s;
        std::getline(std::cin, s);
        if (!s.empty() || optional) return s;
        err = "This field cannot be empty.";
    }
}

int inputInt(const std::string& prompt, int min, int max,
             std::function<void()> reprint) {
    std::string err = "";
    while (true) {
        if (!err.empty()) {
            system("cls");
            if (reprint) reprint();
            std::cout << RED << BOLD << centerPad(err) << "\n\n" << RESET;
        }
        std::cout << CYAN << BOLD << prompt << RESET;
        std::string s;
        std::getline(std::cin, s);
        try {
            int v = std::stoi(s);
            if (v >= min && v <= max) return v;
        } catch (...) {}
        err = "Please enter a number between "
            + std::to_string(min) + " and "
            + std::to_string(max) + ".";
    }
}

float inputFloat(const std::string& prompt, float min, float max,
                 std::function<void()> reprint) {
    std::string err = "";
    while (true) {
        if (!err.empty()) {
            system("cls");
            if (reprint) reprint();
            std::cout << RED << BOLD << centerPad(err) << "\n\n" << RESET;
        }
        std::cout << CYAN << BOLD << prompt << RESET;
        std::string s;
        std::getline(std::cin, s);
        try {
            float v = std::stof(s);
            if (v >= min && v <= max) return v;
        } catch (...) {}
        err = "Please enter a number between "
            + std::to_string((int)min) + " and "
            + std::to_string((int)max) + ".";
    }
}

bool inputYN(const std::string& prompt,
             std::function<void()> reprint) {
    std::string err = "";
    while (true) {
        if (!err.empty()) {
            system("cls");
            if (reprint) reprint();
            std::cout << RED << BOLD << centerPad(err) << "\n\n" << RESET;
        }
        std::cout << CYAN << BOLD << prompt << " (yes/no): " << RESET;
        std::string s;
        std::getline(std::cin, s);
        if (s == "yes" || s == "Yes" || s == "YES") return true;
        if (s == "no"  || s == "No"  || s == "NO")  return false;
        err = "Please enter yes or no.";
    }
}