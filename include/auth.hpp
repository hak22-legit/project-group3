#pragma once
#include <string>
#include <optional>
#include <vector>

std::optional<std::string> loginUser(const std::string& username, const std::string& password);
std::optional<std::string> registerUser(const std::string& username, const std::string& password);
std::vector<std::string> getAllUsers();
bool resetPassword(const std::string& username, const std::string& newPassword);
bool deleteUser(const std::string& username);