#include "omdb.hpp"
#include <nlohmann/json.hpp>
#include <iostream>
#include <vector>
#include <tabulate/tabulate.hpp> 
#define RESET   "\033[0m"
#define CYAN    "\033[36m"
#ifdef HTTPLIB_AVAILABLE
#include "httplib.h"

using json = nlohmann::json;

struct SearchResult {
    std::string imdbID;
    std::string title;
    std::string year;
    std::string type;
    std::string genre;
    std::string director;
};

static std::optional<Entry> fetchByImdbID(httplib::Client& cli, const char* key, const std::string& imdbID) {
    std::string path = std::string("/?apikey=") + key
                     + "&i=" + imdbID
                     + "&plot=short&r=json";

    auto res = cli.Get(path.c_str());
    if (!res || res->status != 200) return std::nullopt;

    json j;
    try { j = json::parse(res->body); } catch (...) { return std::nullopt; }
    if (j.value("Response", "False") == "False") return std::nullopt;

    Entry e;
    e.type       = MediaType::Movie;
    e.title      = j.value("Title",      "");
    e.genre      = j.value("Genre",      "");
    e.director   = j.value("Director",   "");
    e.plot       = j.value("Plot",       "");
    e.imdbRating = j.value("imdbRating", "");
    try { e.year = std::stoi(j.value("Year", "0").substr(0, 4)); } catch (...) {}
    return e;
}

#endif

using json = nlohmann::json;

std::optional<Entry> fetchOMDb(const std::string& title, int year) {
    const char* key = "3da7b947";

#ifndef HTTPLIB_AVAILABLE
    std::cout << "OMDb disabled. Rebuild with -DENABLE_HTTPLIB=ON.\n";
    return std::nullopt;
#else
    std::string enc;
    for (char c : title) enc += (c == ' ') ? '+' : c;

    std::string path = std::string("/?apikey=") + key
                     + "&s=" + enc
                     + (year > 0 ? "&y=" + std::to_string(year) : "")
                     + "&type=movie&r=json";

    httplib::Client cli("http://www.omdbapi.com");
    cli.set_connection_timeout(5);
    auto res = cli.Get(path.c_str());

    if (!res || res->status != 200) {
        std::cout << "OMDb request failed.\n";
        return std::nullopt;
    }

    json j;
    try { j = json::parse(res->body); } catch (...) { return std::nullopt; }

    if (j.value("Response", "False") == "False") {
        std::cout << "OMDb: " << j.value("Error", "Not found") << "\n";
        return std::nullopt;
    }

    std::vector<SearchResult> results;
    for (auto& item : j.value("Search", json::array())) {
        SearchResult r;
        r.imdbID = item.value("imdbID", "");
        r.title  = item.value("Title",  "");
        r.year   = item.value("Year",   "");
        r.type   = item.value("Type",   "");
        if (!r.imdbID.empty()) results.push_back(r);
    }

    if (results.empty()) {
        std::cout << "No results found.\n";
        return std::nullopt;
    }

    // show list
    tabulate::Table resultTable;
    resultTable.add_row({"#", "Title", "Year", "Type"});
    resultTable.add_row({"0", "Cancel", "-", "-"});
    for (int i = 0; i < (int)results.size(); i++) {
        resultTable.add_row({
            std::to_string(i + 1),
            results[i].title,
            results[i].year,
            results[i].type
        });
    }

    resultTable.column(0).format().width(4);
    resultTable.column(1).format().width(35);
    resultTable.column(2).format().width(6);
    resultTable.column(3).format().width(10);
    resultTable.row(0).format().font_style({tabulate::FontStyle::bold});

    for (size_t i = 1; i < resultTable.size(); i++) {
    if (resultTable[i][0].get_text() == "0") {
        resultTable[i][0].format().font_color(tabulate::Color::red);
        resultTable[i][1].format().font_color(tabulate::Color::red);
    } else {
        resultTable[i][0].format().font_color(tabulate::Color::cyan);
        resultTable[i][1].format().font_color(tabulate::Color::cyan);
        resultTable[i][2].format().font_color(tabulate::Color::yellow);
        resultTable[i][3].format().font_color(tabulate::Color::green);
    }
}

    std::cout << "\n" << resultTable << "\n\n";

    int pick = -1;
    while (pick < 0 || pick > (int)results.size()) {
        std::cout << "Pick a result: ";
        std::string s;
        std::getline(std::cin, s);
        try { pick = std::stoi(s); } catch (...) {}
        if (pick < 0 || pick > (int)results.size())
            std::cout << "Enter a number between 0 and " << results.size() << ".\n";
    }

    if (pick == 0) return std::nullopt;

    std::cout << "\n" << CYAN << "Fetching details..." << RESET << "\n\n";
    auto entry = fetchByImdbID(cli, key, results[pick - 1].imdbID);
    if (entry) {
    tabulate::Table preview;
    preview.add_row({"Field", "Value"});
    preview.add_row({"Title",    entry->title});
    preview.add_row({"Year",     entry->year > 0 ? std::to_string(entry->year) : "-"});
    preview.add_row({"Genre",    entry->genre.empty()    ? "-" : entry->genre});
    preview.add_row({"Director", entry->director.empty() ? "-" : entry->director});
    preview.add_row({"IMDb",     entry->imdbRating.empty() ? "-" : entry->imdbRating});
    preview.add_row({"Plot",     entry->plot.empty()     ? "-" : entry->plot});
    preview.column(0).format().width(12);
    preview.column(1).format().width(50);
    preview.row(0).format().font_style({tabulate::FontStyle::bold});
    preview.row(1).format().font_color(tabulate::Color::cyan);
    preview.row(2).format().font_color(tabulate::Color::yellow);
    preview.row(3).format().font_color(tabulate::Color::green);
    preview.row(4).format().font_color(tabulate::Color::magenta);
    preview.row(5).format().font_color(tabulate::Color::yellow);
    preview.row(6).format().font_color(tabulate::Color::white);
    std::cout << preview << "\n\n";
}
return entry;
#endif
}