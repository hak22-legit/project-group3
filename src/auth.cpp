#include "auth.hpp"
#include "storage.hpp"
#include <algorithm>
#include <functional>

// ─── Password Hashing ────────────────────────────────────────────────────────
static std::string hashPassword(const std::string& password) {
    return std::to_string(std::hash<std::string>{}(password));
}

// ─── Register ────────────────────────────────────────────────────────────────
std::optional<std::string> registerUser(const std::string& username, const std::string& password) {
    if (username.empty() || password.empty()) return std::nullopt;
    if (username == "public") return std::nullopt;

    auto users = loadUsers();
    for (auto& u : users)
        if (u.username == username) return std::nullopt;

    users.push_back({ username, hashPassword(password) }); // ✅ store hashed
    saveUsers(users);
    return username;
}

// ─── Login ───────────────────────────────────────────────────────────────────
std::optional<std::string> loginUser(const std::string& username, const std::string& password) {
    auto users = loadUsers();
    for (auto& u : users)
        if (u.username == username && u.password == hashPassword(password)) // ✅ compare hashed
            return username;
    return std::nullopt;
}

// ─── Get All Users ───────────────────────────────────────────────────────────
std::vector<std::string> getAllUsers() {
    auto users = loadUsers();
    std::vector<std::string> result;
    for (auto& u : users)
        result.push_back(u.username);
    return result;
}

// ─── Reset Password ──────────────────────────────────────────────────────────
bool resetPassword(const std::string& username, const std::string& newPassword) {
    if (newPassword.empty()) return false;
    auto users = loadUsers();
    auto it = std::find_if(users.begin(), users.end(),
        [&](const User& u){ return u.username == username; });
    if (it == users.end()) return false;
    it->password = hashPassword(newPassword); // ✅ store hashed
    saveUsers(users);
    return true;
}

// ─── Delete User ─────────────────────────────────────────────────────────────
bool deleteUser(const std::string& username) {
    auto users = loadUsers();
    auto it = std::find_if(users.begin(), users.end(),
        [&](const User& u){ return u.username == username; });
    if (it == users.end()) return false;
    users.erase(it);
    saveUsers(users);
    return true;
}