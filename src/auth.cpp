#include "auth.hpp"
#include "storage.hpp"
#include <algorithm>

std::optional<std::string> registerUser(const std::string& username, const std::string& password) {
    if (username.empty() || password.empty()) return std::nullopt;
    if (username == "public") return std::nullopt;
    auto users = loadUsers();
    for (auto& u : users)
        if (u.username == username) return std::nullopt;
    users.push_back({ username, password });
    saveUsers(users);
    return username;
}

std::optional<std::string> loginUser(const std::string& username, const std::string& password) {
    auto users = loadUsers();
    for (auto& u : users)
        if (u.username == username && u.password == password)
            return username;
    return std::nullopt;
}
