#include "catalog.hpp"
#include "storage.hpp"
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <cctype>
#include <nlohmann/json.hpp>      
#include <tabulate/tabulate.hpp>  
#include <sstream>
#ifdef _WIN32
#include <windows.h>
#endif
          
#define RESET       "\033[0m"
#define BOLD        "\033[1m"
#define DIM         "\033[2m"
#define RED         "\033[31m"
#define GREEN       "\033[32m"
#define YELLOW      "\033[33m"
#define CYAN        "\033[36m"
#define MAGENTA     "\033[35m"
#define WHITE       "\033[37m"

static std::string lower(std::string s) {
    for (char& c : s) c = (char)std::tolower(c);
    return s;
}

static std::string pad(const std::string& s, int w) {
    if ((int)s.size() >= w) return s.substr(0, w);
    return s + std::string(w - s.size(), ' ');
}

static std::string statusStr(const Entry& e) {
    if (e.type == MediaType::Movie)
        return e.status == WatchStatus::Done ? "Watched" : "Not Watched";
    return e.status == WatchStatus::Done ? "Read" : "Want to Read";
}

void addEntry(Catalog& cat, Entry e) {
    e.id = cat.nextId++;
    cat.entries.push_back(e);
    saveCatalog(cat);
}

bool editEntry(Catalog& cat, int id, const Entry& updated) {
    for (auto& e : cat.entries) {
        if (e.id == id) {
            int keep = e.id;
            e = updated;
            e.id = keep;
            saveCatalog(cat);
            return true;
        }
    }
    return false;
}

bool deleteEntry(Catalog& cat, int id) {
    auto prev = cat.entries.size();
    cat.entries.erase(
        std::remove_if(cat.entries.begin(), cat.entries.end(),
            [id](const Entry& e){ return e.id == id; }),
        cat.entries.end());
    if (cat.entries.size() < prev) { saveCatalog(cat); return true; }
    return false;
}

bool toggleStatus(Catalog& cat, int id) {
    for (auto& e : cat.entries) {
        if (e.id == id) {
            e.status = e.status == WatchStatus::Done ? WatchStatus::Pending : WatchStatus::Done;
            saveCatalog(cat);
            return true;
        }
    }
    return false;
}

std::vector<Entry> queryEntries(const Catalog& cat, SortField sort, const FilterOptions& opts) {
    std::vector<Entry> result;
    for (const auto& e : cat.entries) {
        if (!opts.genre.empty()   && lower(e.genre).find(lower(opts.genre))     == std::string::npos) continue;
        if (e.rating < opts.minRating) continue;
        if (!opts.keyword.empty() && lower(e.title).find(lower(opts.keyword))   == std::string::npos) continue;
        result.push_back(e);
    }
    if (sort == SortField::Title)
        std::sort(result.begin(), result.end(), [](const Entry& a, const Entry& b){ return lower(a.title) < lower(b.title); });
    else if (sort == SortField::Year)
        std::sort(result.begin(), result.end(), [](const Entry& a, const Entry& b){ return a.year < b.year; });
    else
        std::sort(result.begin(), result.end(), [](const Entry& a, const Entry& b){ return a.rating > b.rating; });
    return result;
}

void printTable(const std::vector<Entry>& entries, const std::string& label) {
    // ── terminal width ──────────────────────────────
    #ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    int termW = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    #else
    int termW = 80;
    #endif

    // table width = 5+1+28+1+7+1+14+1+6+1+8+1+14 + pipes = 90
    int tableW = 90;
    int tpad   = (termW - tableW) / 2;
    if (tpad < 0) tpad = 0;
    std::string sp = std::string(tpad, ' ');

    // ── helper lambdas ──────────────────────────────
    auto padStr = [](const std::string& s, int w) {
        if ((int)s.size() >= w) return s.substr(0, w);
        return s + std::string(w - s.size(), ' ');
    };

    auto hline = [&](const std::string& fill, const std::string& color) {
        std::cout << sp << color
                  << "+" << std::string(6,  fill[0])
                  << "+" << std::string(29, fill[0])
                  << "+" << std::string(8,  fill[0])
                  << "+" << std::string(15, fill[0])
                  << "+" << std::string(7,  fill[0])
                  << "+" << std::string(9,  fill[0])
                  << "+" << std::string(15, fill[0])
                  << "+" << RESET << "\n";
    };

    auto printRow = [&](
        const std::string& id,
        const std::string& title,
        const std::string& type,
        const std::string& genre,
        const std::string& year,
        const std::string& rating,
        const std::string& status,
        const std::string& borderColor,
        const std::string& idColor,
        const std::string& titleColor,
        const std::string& typeColor,
        const std::string& genreColor,
        const std::string& yearColor,
        const std::string& ratingColor,
        const std::string& statusColor
    ) {
        std::cout << sp
            << borderColor << "|" << RESET
            << " " << idColor      << padStr(id,     5) << RESET << " "
            << borderColor << "|" << RESET
            << " " << titleColor   << padStr(title,  27) << RESET << " "
            << borderColor << "|" << RESET
            << " " << typeColor    << padStr(type,   6) << RESET << " "
            << borderColor << "|" << RESET
            << " " << genreColor   << padStr(genre,  13) << RESET << " "
            << borderColor << "|" << RESET
            << " " << yearColor    << padStr(year,   5) << RESET << " "
            << borderColor << "|" << RESET
            << " " << ratingColor  << padStr(rating, 7) << RESET << " "
            << borderColor << "|" << RESET
            << " " << statusColor  << padStr(status, 13) << RESET << " "
            << borderColor << "|" << RESET << "\n";
    };

    // ── catalog label box ───────────────────────────
    std::string catalogLine = "  Catalog: " + label +
                              " (" + std::to_string(entries.size()) + " entries)  ";
    int boxW = (int)catalogLine.size() + 2;
    std::cout << "\n";
    std::cout << sp << MAGENTA << "+" << std::string(boxW, '=') << "+" << RESET << "\n";
    std::cout << sp << MAGENTA << "|" << RESET
              << BOLD << YELLOW << catalogLine << RESET
              << "  " << MAGENTA << "|" << RESET << "\n";
    std::cout << sp << MAGENTA << "+" << std::string(boxW, '=') << "+" << RESET << "\n\n";

    if (entries.empty()) {
        std::cout << sp << RED << "  No entries found.\n" << RESET << "\n";
        return;
    }

    // ── header ──────────────────────────────────────
    hline("=", CYAN);
    printRow("ID", "Title", "Type", "Genre", "Year", "Rating", "Status",
             CYAN,
             BOLD WHITE, BOLD WHITE, BOLD WHITE, BOLD WHITE,
             BOLD WHITE, BOLD WHITE, BOLD WHITE);
    hline("=", CYAN);

    // ── data rows ───────────────────────────────────
    for (int i = 0; i < (int)entries.size(); i++) {
        const auto& e = entries[i];
        std::string rat = e.rating > 0 ? std::to_string((int)e.rating) + "/10" : "-";
        std::string yr  = e.year   > 0 ? std::to_string(e.year) : "-";
        std::string st  = statusStr(e);

        // row color alternates
        std::string rowColor = (i % 2 == 0) ? YELLOW : WHITE;

        // type color
        std::string typeColor = (e.type == MediaType::Movie) ? MAGENTA : CYAN;

        // rating color
        std::string ratingColor;
        if      (e.rating >= 7.0f) ratingColor = GREEN;
        else if (e.rating >= 5.0f) ratingColor = YELLOW;
        else                       ratingColor = RED;

        // status color
        std::string statusColor = (st == "Watched" || st == "Read") ? GREEN : RED;

        printRow(
            std::to_string(e.id),
            e.title,
            e.type == MediaType::Movie ? "Movie" : "Book",
            e.genre.empty() ? "-" : e.genre,
            yr, rat, st,
            MAGENTA,        // border color
            rowColor,       // id
            rowColor,       // title
            typeColor,      // type
            rowColor,       // genre
            rowColor,       // year
            ratingColor,    // rating
            statusColor     // status
        );
        hline("-", DIM);
    }
    std::cout << "\n";
}

void printEntry(const Entry& e) {
    #ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    int termW = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    #else
    int termW = 80;
    #endif

    int boxW  = 56;
    int tpad  = (termW - boxW) / 2;
    if (tpad < 0) tpad = 0;
    std::string sp = std::string(tpad, ' ');

    std::string rat = e.rating > 0 ? std::to_string((int)e.rating) + "/10" : "-";
    std::string yr  = e.year   > 0 ? std::to_string(e.year) : "-";
    std::string st  = statusStr(e);

    std::string ratingColor;
    if      (e.rating >= 7.0f) ratingColor = GREEN;
    else if (e.rating >= 5.0f) ratingColor = YELLOW;
    else                       ratingColor = RED;

    std::string statusColor = (st == "Watched" || st == "Read") ? GREEN : RED;
    std::string typeColor   = (e.type == MediaType::Movie) ? MAGENTA : CYAN;

    auto printRow = [&](const std::string& label,
                        const std::string& value,
                        const std::string& labelColor,
                        const std::string& valueColor) {
        int labelPad = 12 - (int)label.size();
        if (labelPad < 0) labelPad = 0;
        int valuePad = 39 - (int)value.size();
        if (valuePad < 0) valuePad = 0;

        std::cout << sp
                  << MAGENTA << "║" << RESET
                  << " " << labelColor << BOLD << label << RESET
                  << std::string(labelPad, ' ')
                  << MAGENTA << "║" << RESET
                  << " " << valueColor << value << RESET
                  << std::string(valuePad, ' ')
                  << MAGENTA << "║" << RESET << "\n";
    };

    std::cout << "\n";

    // ── borders — all hardcoded, no std::string(n, multibyte) ──
    // top
    std::cout << sp << MAGENTA << "+" << std::string(54, '=') << "+\n" << RESET;

    // title
    std::string title = "Entry #" + std::to_string(e.id) + " -- " + e.title;
    int titlePad = 54 - (int)title.size() - 1;
    if (titlePad < 0) titlePad = 0;
    std::cout << sp << MAGENTA << "| " << RESET
              << BOLD << CYAN << title << RESET
              << std::string(titlePad, ' ')
              << MAGENTA << "|\n" << RESET;

    // divider
    std::cout << sp << MAGENTA << "+" << std::string(13, '-')
              << "+" << std::string(40, '-') << "+\n" << RESET;

    printRow("Type",     e.type == MediaType::Movie ? "Movie" : "Book", YELLOW, typeColor);
    std::cout << sp << MAGENTA << "+" << std::string(13, '-') << "+" << std::string(40, '-') << "+\n" << RESET;
    printRow("Genre",    e.genre.empty()      ? "-" : e.genre,      YELLOW, WHITE);
    std::cout << sp << MAGENTA << "+" << std::string(13, '-') << "+" << std::string(40, '-') << "+\n" << RESET;
    printRow("Year",     yr,                                          YELLOW, WHITE);
    std::cout << sp << MAGENTA << "+" << std::string(13, '-') << "+" << std::string(40, '-') << "+\n" << RESET;
    printRow("Rating",   rat,                                         YELLOW, ratingColor);
    std::cout << sp << MAGENTA << "+" << std::string(13, '-') << "+" << std::string(40, '-') << "+\n" << RESET;
    printRow("Status",   st,                                          YELLOW, statusColor);
    std::cout << sp << MAGENTA << "+" << std::string(13, '-') << "+" << std::string(40, '-') << "+\n" << RESET;
    printRow("Director", e.director.empty()   ? "-" : e.director,   YELLOW, CYAN);
    std::cout << sp << MAGENTA << "+" << std::string(13, '-') << "+" << std::string(40, '-') << "+\n" << RESET;
    printRow("IMDb",     e.imdbRating.empty() ? "-" : e.imdbRating, YELLOW, GREEN);
    std::cout << sp << MAGENTA << "+" << std::string(13, '-') << "+" << std::string(40, '-') << "+\n" << RESET;
    printRow("Notes",    e.notes.empty()      ? "-" : e.notes,      YELLOW, WHITE);
    std::cout << sp << MAGENTA << "+" << std::string(13, '-') << "+" << std::string(40, '-') << "+\n" << RESET;
    printRow("Plot",     e.plot.empty()       ? "-" : e.plot,       YELLOW, DIM);

    // bottom
    std::cout << sp << MAGENTA << "+" << std::string(54, '=') << "+\n" << RESET;
    std::cout << "\n";
}
