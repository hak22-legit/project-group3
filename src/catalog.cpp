#include "catalog.hpp"
#include "storage.hpp"
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <cctype>
#include <nlohmann/json.hpp>      
#include <tabulate/tabulate.hpp>  
#include <set>
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
#define BLACK       "\033[30m"

static std::string lower(std::string s) {
    for (char& c : s) c = (char)std::tolower(c);
    return s;
}

static std::string pad(const std::string& s, int w) {
    if ((int)s.size() >= w) return s.substr(0, w);
    return s + std::string(w - s.size(), ' ');
}

static std::vector<std::string> wrapText(const std::string& s, int width) {
    std::vector<std::string> lines;
    std::istringstream words(s);
    std::string word, line;
    while (words >> word) {
        if (!line.empty() && (int)(line.size() + 1 + word.size()) > width) {
            lines.push_back(line);
            line = word;
        } else {
            if (!line.empty()) line += ' ';
            line += word;
        }
    }
    if (!line.empty()) lines.push_back(line);
    if (lines.empty()) lines.push_back("");
    return lines;
}


static std::string statusStr(const Entry& e) {
    if (e.type == MediaType::Movie)
        return e.status == WatchStatus::Done ? "Watched" : "Not Watched";
    return e.status == WatchStatus::Done ? "Read" : "Want to Read";
}

void addEntry(Catalog& cat, Entry e) {
    std::set<int> usedIds;
    for (const auto& entry : cat.entries)
        usedIds.insert(entry.id);

    int newId = 1;
    while (usedIds.count(newId))
        newId++;

    e.id = newId;
    cat.nextId = newId + 1;
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
    #ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    int termW = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    #else
    int termW = 80;
    #endif

    int tableW = 90;
    int tpad   = (termW - tableW) / 2;
    if (tpad < 0) tpad = 0;
    std::string sp = std::string(tpad, ' ');

    auto padStr = [](const std::string& s, int w) {
        if ((int)s.size() >= w) return s.substr(0, w);
        return s + std::string(w - s.size(), ' ');
    };

    auto hline = [&](const std::string& fill, const std::string& color) {
        std::cout << sp << color
                  << "+" << std::string(7,  fill[0])
                  << "+" << std::string(29, fill[0])
                //   << "+" << std::string(8,  fill[0])
                  << "+" << std::string(15, fill[0])
                  << "+" << std::string(7,  fill[0])
                  << "+" << std::string(9,  fill[0])
                  << "+" << std::string(15, fill[0])
                  << "+" << RESET << "\n";
    };

    auto printRow = [&](
        const std::string& id,
        const std::string& title,
        // const std::string& type,
        const std::string& genre,
        const std::string& year,
        const std::string& rating,
        const std::string& status,
        const std::string& borderColor,
        const std::string& idColor,
        const std::string& titleColor,
        // const std::string& typeColor,
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
            // << borderColor << "|" << RESET
            // << " " << typeColor    << padStr(type,   6) << RESET << " "
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

    hline("=", CYAN);
    printRow("ID", "Title", "Genre", "Year", "Rating", "Status",
             CYAN,
             BOLD CYAN, BOLD CYAN, BOLD CYAN, BOLD CYAN,
             BOLD CYAN, BOLD CYAN);
    hline("=", CYAN);

    for (int i = 0; i < (int)entries.size(); i++) {
        const auto& e = entries[i];
        std::string rat = e.rating > 0 ? std::to_string((int)e.rating) + "/10" : "-";
        std::string yr  = e.year   > 0 ? std::to_string(e.year) : "-";
        std::string st  = statusStr(e);

        std::string rowColor    = (i % 2 == 0) ? YELLOW : CYAN;
        std::string typeColor   = (e.type == MediaType::Movie) ? MAGENTA : CYAN;
        std::string ratingColor;
        if      (e.rating >= 7.0f) ratingColor = GREEN;
        else if (e.rating >= 5.0f) ratingColor = YELLOW;
        else                       ratingColor = RED;
        std::string statusColor = (st == "Watched" || st == "Read") ? GREEN : RED;

        printRow(
            std::to_string(e.id),
            e.title,
            // e.type == MediaType::Movie ? "Movie" : "Book",
            e.genre.empty() ? "-" : e.genre,
            yr, rat, st,
            MAGENTA,
            rowColor, rowColor, rowColor, rowColor, ratingColor, statusColor
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

    // value column width = 39 chars
    const int VALUE_W = 39;

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

    // ── single-line row (all fields except Plot) ──────────────────────────────
    auto printRow = [&](const std::string& label,
                        const std::string& value,
                        const std::string& labelColor,
                        const std::string& valueColor) {
        int labelPad = 12 - (int)label.size();
        if (labelPad < 0) labelPad = 0;
        // truncate value to VALUE_W so it never overflows the box
        std::string val = (int)value.size() > VALUE_W
                          ? value.substr(0, VALUE_W)
                          : value;
        int valuePad = VALUE_W - (int)val.size();
        if (valuePad < 0) valuePad = 0;

        std::cout << sp
                  << MAGENTA << "║" << RESET
                  << " " << labelColor << BOLD << label << RESET
                  << std::string(labelPad, ' ')
                  << MAGENTA << "║" << RESET
                  << " " << valueColor << val << RESET
                  << std::string(valuePad, ' ')
                  << MAGENTA << "║" << RESET << "\n";
    };


    auto printPlotRow = [&](const std::string& label,
                            const std::string& value,
                            const std::string& labelColor,
                            const std::string& valueColor) {
        std::string plotVal = value.empty() ? "-" : value;
        auto lines = wrapText(plotVal, VALUE_W);

        int labelPad = 12 - (int)label.size();
        if (labelPad < 0) labelPad = 0;

        for (int i = 0; i < (int)lines.size(); i++) {
            int vp = VALUE_W - (int)lines[i].size();
            if (vp < 0) vp = 0;

            std::cout << sp << MAGENTA << "║" << RESET;

            if (i == 0)
                // first line: show label
                std::cout << " " << labelColor << BOLD << label << RESET
                          << std::string(labelPad, ' ');
            else
                // continuation lines: blank label area (keeps columns aligned)
                std::cout << " " << std::string(12, ' ');

            std::cout << MAGENTA << "║" << RESET
                      << " " << valueColor << lines[i] << RESET
                      << std::string(vp, ' ')
                      << MAGENTA << "║" << RESET << "\n";
            // NO divider between wrapped lines — clean multi-line appearance
        }
    };

    auto divider = [&]() {
        std::cout << sp << MAGENTA << "+"
                  << std::string(13, '-') << "+"
                  << std::string(40, '-') << "+\n" << RESET;
    };

    std::cout << "\n";

    // top border
    std::cout << sp << MAGENTA << "+" << std::string(54, '=') << "+\n" << RESET;

    // title bar
    std::string title = "Entry #" + std::to_string(e.id) + " -- " + e.title;
    int titlePad = 54 - (int)title.size() - 1;
    if (titlePad < 0) titlePad = 0;
    std::cout << sp << MAGENTA << "| " << RESET
              << BOLD << CYAN << title << RESET
              << std::string(titlePad, ' ')
              << MAGENTA << "|\n" << RESET;

    divider();
    printRow("Type",     e.type == MediaType::Movie ? "Movie" : "Book", YELLOW, typeColor);
    divider();
    printRow("Genre",    e.genre.empty()      ? "-" : e.genre,      YELLOW, BLACK);
    divider();
    printRow("Year",     yr,                                          YELLOW, BLACK);
    divider();
    printRow("Rating",   rat,                                         YELLOW, ratingColor);
    divider();
    printRow("Status",   st,                                          YELLOW, statusColor);
    divider();
    printRow("Director", e.director.empty()   ? "-" : e.director,   YELLOW, CYAN);
    divider();
    printRow("IMDb",     e.imdbRating.empty() ? "-" : e.imdbRating, YELLOW, GREEN);
    divider();
    printRow("Notes",    e.notes.empty()      ? "-" : e.notes,      YELLOW, BLACK);
    divider();
    // ── Plot: word-wrapped across multiple rows ───────────────────────────────
    printPlotRow("Plot", e.plot, YELLOW, DIM);

    // bottom border
    std::cout << sp << MAGENTA << "+" << std::string(54, '=') << "+\n" << RESET;
    std::cout << "\n";
}
