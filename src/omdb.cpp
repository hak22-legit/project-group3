#include "omdb.hpp"
#include <nlohmann/json.hpp>
#include <iostream>
#include <vector>
#include <sstream>
#include <chrono>
#include <thread>
#include <tabulate/tabulate.hpp>
#ifdef _WIN32
#include <windows.h>
#endif

// ─── ANSI Colors ─────────────────────────────────────────────────────────────
#define RESET    "\033[0m"
#define BOLD     "\033[1m"
#define DIM      "\033[2m"
#define RED      "\033[31m"
#define GREEN    "\033[32m"
#define YELLOW   "\033[33m"
#define MAGENTA  "\033[35m"
#define CYAN     "\033[36m"
#define WHITE    "\033[37m"

// ─── Terminal Width Helper ────────────────────────────────────────────────────
static int getTermWidthOMDb() {
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    return csbi.srWindow.Right - csbi.srWindow.Left + 1;
#else
    return 80;
#endif
}

static std::string centerPadOMDb(const std::string& text) {
    int termWidth = getTermWidthOMDb();
    int pad = (termWidth - (int)text.size()) / 2;
    if (pad < 0) pad = 0;
    return std::string(pad, ' ') + text;
}

#ifdef HTTPLIB_AVAILABLE
#include "httplib.h"

using json = nlohmann::json;

struct SearchResult {
    std::string imdbID;
    std::string title;
    std::string year;
    std::string type;
};

static std::optional<Entry> fetchByImdbID(httplib::Client& cli,
                                           const char* key,
                                           const std::string& imdbID) {
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
    std::cout << centerPadOMDb("OMDb disabled. Rebuild with -DENABLE_HTTPLIB=ON.") << "\n";
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
        std::cout << RED << BOLD
                  << centerPadOMDb("OMDb request failed.") << RESET << "\n";
        return std::nullopt;
    }

    json j;
    try { j = json::parse(res->body); } catch (...) { return std::nullopt; }

    if (j.value("Response", "False") == "False") {
        std::cout << RED << BOLD
                  << centerPadOMDb("OMDb: " + j.value("Error", "Not found"))
                  << RESET << "\n";
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
        std::cout << RED << BOLD
                  << centerPadOMDb("No results found.") << RESET << "\n";
        return std::nullopt;
    }

    // ── wrap text helper ──────────────────────────
    auto wrapText = [](const std::string& s, int maxLen) -> std::vector<std::string> {
        std::vector<std::string> lines;
        std::string remaining = s;
        while ((int)remaining.size() > maxLen) {
            int cut = maxLen;
            for (int i = maxLen - 1; i >= 0; i--) {
                if (remaining[i] == ' ') { cut = i; break; }
            }
            lines.push_back(remaining.substr(0, cut));
            remaining = remaining.substr(cut + (remaining[cut] == ' ' ? 1 : 0));
        }
        if (!remaining.empty()) lines.push_back(remaining);
        return lines;
    };

    // ── result table centered ─────────────────────
    int termW2 = getTermWidthOMDb();
    int boxW2  = 60;
    int rpad   = (termW2 - boxW2) / 2;
    if (rpad < 0) rpad = 0;
    std::string rsp = std::string(rpad, ' ');

    auto rHline = [&](char fill, const std::string& color) {
        std::cout << rsp << color
                  << "+" << std::string(5,  fill)
                  << "+" << std::string(36, fill)
                  << "+" << std::string(7,  fill)
                  << "+" << std::string(11, fill)
                  << "+" << RESET << "\n";
    };

    auto rRow = [&](const std::string& num,
                    const std::string& title,
                    const std::string& year,
                    const std::string& type,
                    const std::string& numColor,
                    const std::string& titleColor,
                    const std::string& yearColor,
                    const std::string& typeColor,
                    const std::string& borderColor) {

        auto titleLines = wrapText(title, 35);

        int np = 4  - (int)num.size();
        int yp = 6  - (int)year.size();
        int xp = 10 - (int)type.size();
        if (np < 0) np = 0;
        if (yp < 0) yp = 0;
        if (xp < 0) xp = 0;

        // first line
        int tp0 = 35 - (int)titleLines[0].size();
        if (tp0 < 0) tp0 = 0;
        std::cout << rsp
                  << borderColor << "|" << RESET
                  << " " << numColor << BOLD << num << RESET
                  << std::string(np, ' ')
                  << borderColor << "|" << RESET
                  << " " << titleColor << titleLines[0] << RESET
                  << std::string(tp0, ' ')
                  << borderColor << "|" << RESET
                  << " " << yearColor << year << RESET
                  << std::string(yp, ' ')
                  << borderColor << "|" << RESET
                  << " " << typeColor << type << RESET
                  << std::string(xp, ' ')
                  << borderColor << "|" << RESET << "\n";

        // continuation lines
        for (int i = 1; i < (int)titleLines.size(); i++) {
            int tp = 35 - (int)titleLines[i].size();
            if (tp < 0) tp = 0;
            std::cout << rsp
                      << borderColor << "|" << RESET
                      << " " << std::string(4, ' ')
                      << borderColor << "|" << RESET
                      << " " << titleColor << titleLines[i] << RESET
                      << std::string(tp, ' ')
                      << borderColor << "|" << RESET
                      << " " << std::string(6,  ' ')
                      << borderColor << "|" << RESET
                      << " " << std::string(10, ' ')
                      << borderColor << "|" << RESET << "\n";
        }
    };

    std::cout << "\n";
    rHline('=', CYAN);
    rRow("#", "Title", "Year", "Type",
         BOLD WHITE, BOLD WHITE, BOLD WHITE, BOLD WHITE, CYAN);
    rHline('=', CYAN);
    rRow("0", "Cancel", "-", "-",
         RED, RED, RED, RED, MAGENTA);
    rHline('-', MAGENTA);
    for (int i = 0; i < (int)results.size(); i++) {
        rRow(std::to_string(i + 1),
             results[i].title,
             results[i].year,
             results[i].type,
             CYAN, CYAN, YELLOW, GREEN, MAGENTA);
        if (i < (int)results.size() - 1)
            rHline('-', MAGENTA);
    }
    rHline('=', CYAN);
    std::cout << "\n";

    // ── pick prompt ───────────────────────────────
   int pick = -1;
    while (pick < 0 || pick > (int)results.size()) {
        std::cout << CYAN << BOLD
                  << centerPadOMDb("Pick a result (0-"
                     + std::to_string(results.size()) + "): ")
                  << RESET;
        std::string s;
        std::getline(std::cin, s);
        try { pick = std::stoi(s); } catch (...) { pick = -1; }
        if (pick < 0 || pick > (int)results.size()) {
            std::cout << "\n" << RED << BOLD
                      << centerPadOMDb("Invalid choice! Please enter 0 to "
                         + std::to_string(results.size()) + ".")
                      << RESET << "\n\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            system("cls");

            // reprint table after cls
            std::cout << "\n";
            rHline('=', CYAN);
            rRow("#", "Title", "Year", "Type",
                 BOLD WHITE, BOLD WHITE, BOLD WHITE, BOLD WHITE, CYAN);
            rHline('=', CYAN);
            rRow("0", "Cancel", "-", "-",
                 RED, RED, RED, RED, MAGENTA);
            rHline('-', MAGENTA);
            for (int i = 0; i < (int)results.size(); i++) {
                rRow(std::to_string(i + 1),
                     results[i].title,
                     results[i].year,
                     results[i].type,
                     CYAN, CYAN, YELLOW, GREEN, MAGENTA);
                if (i < (int)results.size() - 1)
                    rHline('-', MAGENTA);
            }
            rHline('=', CYAN);
            std::cout << "\n";
            pick = -1;
        }
    }

    if (pick == 0) return std::nullopt;

    // ── fetching message ──────────────────────────
    std::cout << "\n" << CYAN << BOLD
              << centerPadOMDb("Fetching details...") << RESET << "\n\n";

    auto entry = fetchByImdbID(cli, key, results[pick - 1].imdbID);

    if (entry) {
        int termW3 = getTermWidthOMDb();
        int pboxW  = 66;
        int ppad   = (termW3 - pboxW) / 2;
        if (ppad < 0) ppad = 0;
        std::string psp = std::string(ppad, ' ');

        auto pHline = [&](char fill, const std::string& color) {
            std::cout << psp << color
                      << "+" << std::string(13, fill)
                      << "+" << std::string(51, fill)
                      << "+" << RESET << "\n";
        };

        auto pRow = [&](const std::string& label,
                        const std::string& value,
                        const std::string& labelColor,
                        const std::string& valueColor,
                        const std::string& borderColor) {

            auto valueLines = wrapText(value, 50);

            int lp  = 12 - (int)label.size();
            if (lp < 0) lp = 0;

            // first line
            int vp0 = 50 - (int)valueLines[0].size();
            if (vp0 < 0) vp0 = 0;
            std::cout << psp
                      << borderColor << "|" << RESET
                      << " " << labelColor << BOLD << label << RESET
                      << std::string(lp, ' ')
                      << borderColor << "|" << RESET
                      << " " << valueColor << valueLines[0] << RESET
                      << std::string(vp0, ' ')
                      << borderColor << "|" << RESET << "\n";

            // continuation lines
            for (int i = 1; i < (int)valueLines.size(); i++) {
                int vp = 50 - (int)valueLines[i].size();
                if (vp < 0) vp = 0;
                std::cout << psp
                          << borderColor << "|" << RESET
                          << " " << std::string(12, ' ')
                          << borderColor << "|" << RESET
                          << " " << valueColor << valueLines[i] << RESET
                          << std::string(vp, ' ')
                          << borderColor << "|" << RESET << "\n";
            }
        };

        std::string previewYr = entry->year > 0
                              ? std::to_string(entry->year) : "-";

        std::cout << "\n";
        pHline('=', CYAN);
        pRow("Field",    "Value",                                   WHITE,   WHITE,   CYAN);
        pHline('=', CYAN);
        pRow("Title",    entry->title,                              YELLOW,  CYAN,    MAGENTA);
        pHline('-', MAGENTA);
        pRow("Year",     previewYr,                                 YELLOW,  YELLOW,  MAGENTA);
        pHline('-', MAGENTA);
        pRow("Genre",    entry->genre.empty()
                         ? "-" : entry->genre,                      YELLOW,  GREEN,   MAGENTA);
        pHline('-', MAGENTA);
        pRow("Director", entry->director.empty()
                         ? "-" : entry->director,                   YELLOW,  MAGENTA, MAGENTA);
        pHline('-', MAGENTA);
        pRow("IMDb",     entry->imdbRating.empty()
                         ? "-" : entry->imdbRating,                 YELLOW,  GREEN,   MAGENTA);
        pHline('-', MAGENTA);
        pRow("Plot",     entry->plot.empty()
                         ? "-" : entry->plot,                       YELLOW,  WHITE,   MAGENTA);
        pHline('=', CYAN);
        std::cout << "\n";
    }

    return entry;
#endif
}