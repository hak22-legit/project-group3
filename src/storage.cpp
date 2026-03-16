#include "storage.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;
using json   = nlohmann::json;

static std::string catalogPath(const std::string& username) {
    return "data/" + username + "/catalog.json";
}

static MediaType   toType(const std::string& s)  { return s == "book" ? MediaType::Book : MediaType::Movie; }
static std::string fromType(MediaType t)          { return t == MediaType::Book ? "book" : "movie"; }
static WatchStatus toStatus(const std::string& s) { return s == "done" ? WatchStatus::Done : WatchStatus::Pending; }
static std::string fromStatus(WatchStatus s)      { return s == WatchStatus::Done ? "done" : "pending"; }

static Entry entryFromJson(const json& j) {
    Entry e;
    e.id         = j.value("id",         0);
    e.title      = j.value("title",      "");
    e.type       = toType(j.value("type","movie"));
    e.genre      = j.value("genre",      "");
    e.year       = j.value("year",       0);
    e.rating     = j.value("rating",     0.0f);
    e.notes      = j.value("notes",      "");
    e.status     = toStatus(j.value("status","pending"));
    e.director   = j.value("director",   "");
    e.plot       = j.value("plot",       "");
    e.imdbRating = j.value("imdbRating", "");
    return e;
}

static json entryToJson(const Entry& e) {
    return json{
        {"id",         e.id},
        {"title",      e.title},
        {"type",       fromType(e.type)},
        {"genre",      e.genre},
        {"year",       e.year},
        {"rating",     e.rating},
        {"notes",      e.notes},
        {"status",     fromStatus(e.status)},
        {"director",   e.director},
        {"plot",       e.plot},
        {"imdbRating", e.imdbRating}
    };
}

std::vector<User> loadUsers() {
    std::vector<User> users;
    fs::create_directories("data");
    std::ifstream f("data/users.json");
    if (!f.is_open()) return users;
    json j;
    try { f >> j; } catch (...) { return users; }
    for (auto& u : j)
        users.push_back({ u.value("username",""), u.value("password","") });
    return users;
}

void saveUsers(const std::vector<User>& users) {
    fs::create_directories("data");
    json j = json::array();
    for (auto& u : users)
        j.push_back({ {"username", u.username}, {"password", u.password} });
    std::ofstream f("data/users.json");
    f << j.dump(2);
}

Catalog loadCatalog(const std::string& username) {
    Catalog cat;
    cat.username = username;
    fs::create_directories("data/" + username);
    std::ifstream f(catalogPath(username));
    if (!f.is_open()) return cat;
    json j;
    try { f >> j; } catch (...) { return cat; }
    cat.nextId = j.value("nextId", 1);
    for (auto& ej : j.value("entries", json::array()))
        cat.entries.push_back(entryFromJson(ej));
    return cat;
}

void saveCatalog(const Catalog& cat) {
    fs::create_directories("data/" + cat.username);
    json j;
    j["nextId"]  = cat.nextId;
    j["entries"] = json::array();
    for (auto& e : cat.entries)
        j["entries"].push_back(entryToJson(e));
    std::ofstream f(catalogPath(cat.username));
    f << j.dump(2);
}
