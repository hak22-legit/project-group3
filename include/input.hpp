#pragma once
#include <string>
#include <functional>

std::string inputLine (const std::string& prompt, bool optional = false,
                       std::function<void()> reprint = nullptr);
int inputInt(const std::string& prompt, int min, int max,
             std::function<void()> reprint = nullptr);
float       inputFloat(const std::string& prompt, float min, float max,
                       std::function<void()> reprint = nullptr);
bool        inputYN   (const std::string& prompt,
                       std::function<void()> reprint = nullptr);