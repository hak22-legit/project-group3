#pragma once
#include <string>
#include <vector>

enum class MediaType   { Movie, Book };
enum class WatchStatus { Done, Pending };

struct Entry {
    int         id       = 0;
    std::string title;
    MediaType   type     = MediaType::Movie;
    std::string genre;
    int         year     = 0;
    float       rating   = 0.0f;
    std::string notes;
    WatchStatus status   = WatchStatus::Pending;
    std::string director;
    std::string plot;
    std::string imdbRating;
};

struct Catalog {
    std::string        username;
    std::vector<Entry> entries;
    int                nextId = 1;
};

struct User {
    std::string username;
    std::string password;
};
