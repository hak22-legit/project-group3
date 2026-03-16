#pragma once
#include <string>

std::string inputLine(const std::string& prompt, bool allowEmpty = false);
int         inputInt(const std::string& prompt, int min, int max);
float       inputFloat(const std::string& prompt, float min, float max);
bool        inputYN(const std::string& prompt);
