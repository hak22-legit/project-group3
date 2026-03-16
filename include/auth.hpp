#pragma once
#include <string>
#include <optional>

std::optional<std::string> loginUser(const std::string& username, const std::string& password);
std::optional<std::string> registerUser(const std::string& username, const std::string& password);
