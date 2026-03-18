#include "input.hpp"
#include <iostream>
#include <limits>

#define RESET   "\033[0m"
#define BOLD    "\033[1m"
#define CYAN    "\033[36m"
#define RED     "\033[31m"
#define YELLOW  "\033[33m"

std::string inputLine(const std::string& prompt, bool optional) {
    while (true) {
        std::cout << CYAN << BOLD << prompt << RESET;
        std::string s;
        std::getline(std::cin, s);
        if (!s.empty() || optional) return s;
        std::cout << RED << "This field cannot be empty.\n" << RESET;
    }
}

int inputInt(const std::string& prompt, int min, int max) {
    while (true) {
        std::cout << CYAN << BOLD << prompt << RESET;
        std::string s;
        std::getline(std::cin, s);
        try {
            int v = std::stoi(s);
            if (v >= min && v <= max) return v;
        } catch (...) {}
        std::cout << RED << "Please enter a number between "
                  << min << " and " << max << ".\n" << RESET;
    }
}

float inputFloat(const std::string& prompt, float min, float max) {
    while (true) {
        std::cout << CYAN << BOLD << prompt << RESET;
        std::string s;
        std::getline(std::cin, s);
        try {
            float v = std::stof(s);
            if (v >= min && v <= max) return v;
        } catch (...) {}
        std::cout << RED << "Please enter a number between "
                  << min << " and " << max << ".\n" << RESET;
    }
}

bool inputYN(const std::string& prompt) {
    while (true) {
        std::cout << CYAN << BOLD << prompt << " (yes/no): " << RESET;
        std::string s;
        std::getline(std::cin, s);
        if (s == "yes" || s == "Yes" || s == "YES") return true;
        if (s == "no"  || s == "No"  || s == "NO")  return false;
        std::cout << RED << "Please enter yes or no.\n" << RESET;
    }
}