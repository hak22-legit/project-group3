#include "input.hpp"
#include <iostream>

std::string inputLine(const std::string& prompt, bool allowEmpty) {
    std::string val;
    while (true) {
        std::cout << prompt;
        std::getline(std::cin, val);
        while (!val.empty() && (val.back() == '\r' || val.back() == '\n')) val.pop_back();
        if (allowEmpty || !val.empty()) return val;
        std::cout << "Cannot be empty. Try again.\n";
    }
}

int inputInt(const std::string& prompt, int min, int max) {
    while (true) {
        std::cout << prompt;
        std::string s;
        std::getline(std::cin, s);
        try {
            int v = std::stoi(s);
            if (v >= min && v <= max) return v;
        } catch (...) {}
        std::cout << "Enter years between " << min << " and " << max << ".\n";
    }
}

float inputFloat(const std::string& prompt, float min, float max) {
    while (true) {
        std::cout << prompt;
        std::string s;
        std::getline(std::cin, s);
        try {
            float v = std::stof(s);
            if (v >= min && v <= max) return v;
        } catch (...) {}
        std::cout << "Enter rating between " << min << " and " << max << ".\n";
    }
}

bool inputYN(const std::string& prompt) {
    while (true) {
        std::cout << prompt << " (yes/no): ";
        std::string s;
        std::getline(std::cin, s);
        if (s == "yes" || s == "Yes") return true;
        if (s == "no" || s == "No") return false;
        std::cout << "Please enter yes or no.\n";
    }
}
