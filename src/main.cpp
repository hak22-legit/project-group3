#include "types.hpp"
#include "auth.hpp"
#include "storage.hpp"
#include "catalog.hpp"
#include "input.hpp"
#include "omdb.hpp"
#include <iostream>
#include <algorithm>
#include <map>
#include <limits>
#include <chrono>
#include <thread>
#include <tabulate/tabulate.hpp>
#ifdef _WIN32
#include <windows.h>
#endif

// ─── ANSI Color Codes ───────────────────────────────────────────────────────
#define RESET       "\033[0m"
#define BOLD        "\033[1m"
#define DIM         "\033[2m"
#define RED         "\033[31m"
#define GREEN       "\033[32m"
#define YELLOW      "\033[33m"
#define BLUE        "\033[34m"
#define MAGENTA     "\033[35m"
#define CYAN        "\033[36m"
#define WHITE       "\033[37m"
#define BG_BLUE     "\033[44m"
#define BG_GREY     "\033[100m"
// ────────────────────────────────────────────────────────────────────────────

static void printGoodbye() {
    std::cout << "\n\n";
    std::cout << RED << BOLD;
    std::cout << "   ██████╗  ██████╗  ██████╗ ██████╗ ██████╗ ██╗   ██╗███████╗██╗\n";
    std::cout << "  ██╔════╝ ██╔═══██╗██╔═══██╗██╔══██╗██╔══██╗╚██╗ ██╔╝██╔════╝██║\n";
    std::cout << "  ██║  ███╗██║   ██║██║   ██║██║  ██║██████╔╝ ╚████╔╝ █████╗  ██║\n";
    std::cout << "  ██║   ██║██║   ██║██║   ██║██║  ██║██╔══██╗  ╚██╔╝  ██╔══╝  ╚═╝\n";
    std::cout << "  ╚██████╔╝╚██████╔╝╚██████╔╝██████╔╝██████╔╝   ██║   ███████╗██╗\n";
    std::cout << "   ╚═════╝  ╚═════╝  ╚═════╝ ╚═════╝ ╚═════╝    ╚═╝   ╚══════╝╚═╝\n";
    std::cout << RESET << "\n";
    std::cout << YELLOW << BOLD;
    std::cout << "              Thanks for using Movie catalog!\n";
    std::cout << "                    See you next time :)\n";
    std::cout << RESET << "\n\n";
}

static void printLoading(const std::string& msg = "Loading", int termWidth = 150) {
    int barWidth = 40;
    int pad = (termWidth - barWidth - 12) / 2;
    std::string sp = std::string(pad, ' ');

    std::cout << "\n" << sp << BOLD << YELLOW << msg << RESET << "\n";

    for (int i = 0; i <= barWidth; i++) {
        std::cout << "\r" << sp << CYAN << "Loading: [" << RESET;
        std::cout << GREEN << BOLD;
        for (int j = 0; j < i; j++) std::cout << "\xe2\x96\x88";
        for (int j = i; j < barWidth; j++) std::cout << " ";
        std::cout << RESET << CYAN << "] " << RESET
                  << BOLD << (i * 100 / barWidth) << "%" << RESET
                  << std::flush;
        std::this_thread::sleep_for(std::chrono::milliseconds(40));
    }
    std::cout << "\n" << sp << GREEN << BOLD << "Done!" << RESET << "\n\n";
}

static void printCentered(const std::string& text, int termWidth = 80) {
    int pad = (termWidth - (int)text.size()) / 2;
    if (pad < 0) pad = 0;
    std::cout << std::string(pad, ' ') << text << "\n";
}

static void printAnimated(const std::string& text, int delayMs = 50) {
    for (char c : text) {
        std::cout << c << std::flush;
        std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
    }
    std::cout << "\n";
}

static void printTitle() {
    int w = 80;
    auto center = [&](const std::string& s, int w = 154) {
        int pad = (w - (int)s.size()) / 2;
        if (pad < 0) pad = 0;
        std::cout << std::string(pad, ' ') << s << "\n";
    };

    std::cout << GREEN << BOLD;
    center("################################################################################");
    center("#                                                                              #");
    center("#                        MEDIA CATALOG MANAGER                                 #");
    center("#                                                                              #");
    center("#                     Your personal movie tracker                              #");
    center("#                                                                              #");
    center("################################################################################");
    std::cout << RESET << "\n";
    std::cout << GREEN;
    int pad = (196 - 50) / 2;
    std::cout << std::string(pad, ' ');
    printAnimated("Developed by Pre-Gen6 Group3", 60);
    std::cout << RESET << "\n";
}

static void printDivider(int width = 44) {
    std::cout << DIM << std::string(width, '-') << RESET << "\n";
}

static void printHeader(const std::string& emoji, const std::string& title) {
    std::cout << "\n";
    printDivider();
    std::cout << BOLD << CYAN << "  " << emoji << "  " << title << RESET << "\n";
    printDivider();
    std::cout << "\n";
}

static Entry* findEntry(Catalog& cat, int id) {
    for (auto& e : cat.entries)
        if (e.id == id) return &e;
    return nullptr;
}

static SortField askSort() {
    std::cout << YELLOW << "Sort by: " << RESET
              << CYAN << "1" << RESET << ".Title  "
              << CYAN << "2" << RESET << ".Year  "
              << CYAN << "3" << RESET << ".Rating: ";
    int s = inputInt("", 1, 3);
    return s == 1 ? SortField::Title : s == 2 ? SortField::Year : SortField::Rating;
}

// ─── List ────────────────────────────────────────────────────────────────────
static void doList(const Catalog& cat) {
    printHeader(">>", "CATALOG LIST");
    printTable(queryEntries(cat, askSort(), {}), cat.username);
}

// ─── View ────────────────────────────────────────────────────────────────────
static void doView(Catalog& cat) {
    printHeader("VIEW", "VIEW ENTRY");
    int id = inputInt("Entry ID: ", 1, 99999);
    Entry* ep = nullptr;
    for (auto& e : cat.entries)
        if (e.id == id) { ep = &e; break; }
    if (!ep) { std::cout << RED << "Entry not found.\n" << RESET; return; }
    printEntry(*ep);
    if (ep->type == MediaType::Movie && inputYN("Fetch OMDb data for this entry?")) {
        auto res = fetchOMDb(ep->title, ep->year);
        if (res) {
            ep->director   = res->director;
            ep->plot       = res->plot;
            ep->imdbRating = res->imdbRating;
            if (ep->genre.empty()) ep->genre = res->genre;
            if (ep->year  == 0)    ep->year  = res->year;
            saveCatalog(cat);
            std::cout << "\n" << GREEN << BOLD << "OMDb data saved!" << RESET << "\n\n";
            printEntry(*ep);
        }
    }
}

// ─── Add ─────────────────────────────────────────────────────────────────────
static void doAdd(Catalog& cat) {
    printHeader("+", "ADD NEW ENTRY");

    tabulate::Table form;
    form.add_row({"Field", "Description"});
    form.add_row({"Title",   "Enter movie or book title"});
    form.add_row({"Type",    "1 = Movie   2 = Book"});
    form.add_row({"Genre",   "e.g. Action, Sci-Fi, Drama"});
    form.add_row({"Year",    "1888 - 2100"});
    form.add_row({"Rating",  "1.0 - 10.0"});
    form.add_row({"Notes",   "Any personal notes (optional)"});
    form.add_row({"Status",  "Watched / Not Watched"});
    form.column(0).format().width(14);
    form.column(1).format().width(32);
    form.row(0).format().font_style({tabulate::FontStyle::bold});
    std::cout << form << "\n\n";

    Entry e;
    e.title  = inputLine("Title   : ");
    int t    = inputInt("Type  1=Movie  2=Book : ", 1, 2);
    e.type   = t == 1 ? MediaType::Movie : MediaType::Book;
    e.genre  = inputLine("Genre   : ", true);
    e.year   = inputInt("Year    : ", 1888, 2100);
    e.rating = inputFloat("Rating  : ", 1.0f, 10.0f);
    e.notes  = inputLine("Notes   : ", true);
    std::string label = e.type == MediaType::Movie ? "Watched?" : "Read?";
    e.status = inputYN(label) ? WatchStatus::Done : WatchStatus::Pending;

    addEntry(cat, e);
    std::cout << "\n" << BOLD << GREEN << "ENTRY SAVED!" << RESET << "\n\n";

    tabulate::Table summary;
    summary.add_row({"Field", "Value"});
    summary.add_row({"ID",     std::to_string(cat.entries.back().id)});
    summary.add_row({"Title",  e.title});
    summary.add_row({"Type",   e.type == MediaType::Movie ? "Movie" : "Book"});
    summary.add_row({"Genre",  e.genre.empty() ? "-" : e.genre});
    summary.add_row({"Year",   std::to_string(e.year)});
    summary.add_row({"Rating", std::to_string((int)e.rating) + "/10"});
    summary.add_row({"Status", e.status == WatchStatus::Done
                        ? (e.type == MediaType::Movie ? "Watched" : "Read")
                        : (e.type == MediaType::Movie ? "Not Watched" : "Want to Read")});
    summary.add_row({"Notes",  e.notes.empty() ? "-" : e.notes});
    summary.column(0).format().width(14);
    summary.column(1).format().width(30);
    summary.row(0).format().font_style({tabulate::FontStyle::bold});
    std::cout << summary << "\n\n";
}

// ─── Edit ────────────────────────────────────────────────────────────────────
static void doEdit(Catalog& cat) {
    printHeader("EDIT", "EDIT ENTRY");
    int id = inputInt("Entry ID to edit: ", 1, 99999);
    Entry* ep = findEntry(cat, id);
    if (!ep) { std::cout << RED << "Entry not found.\n" << RESET; return; }
    printEntry(*ep);
    std::cout << YELLOW << "Press Enter to keep existing value.\n\n" << RESET;
    Entry updated = *ep;
    std::string s;

    std::cout << "Title  [" << CYAN << updated.title  << RESET << "]: ";
    std::getline(std::cin, s); if (!s.empty()) updated.title = s;

    std::cout << "Genre  [" << CYAN << updated.genre  << RESET << "]: ";
    std::getline(std::cin, s); if (!s.empty()) updated.genre = s;

    std::cout << "Year   [" << CYAN << updated.year   << RESET << "]: ";
    std::getline(std::cin, s);
    if (!s.empty()) try { updated.year = std::stoi(s); } catch (...) {}

    std::cout << "Rating [" << CYAN << updated.rating << RESET << "]: ";
    std::getline(std::cin, s);
    if (!s.empty()) try {
        float v = std::stof(s);
        if (v >= 0 && v <= 10) updated.rating = v;
    } catch (...) {}

    std::cout << "Notes  [" << CYAN << updated.notes  << RESET << "]: ";
    std::getline(std::cin, s); if (!s.empty()) updated.notes = s;

    if (editEntry(cat, id, updated))
        std::cout << "\n" << GREEN << BOLD << "Entry updated!" << RESET << "\n\n";
    else
        std::cout << "\n" << RED << "Update failed.\n" << RESET;
}

// ─── Delete ──────────────────────────────────────────────────────────────────
static void doDelete(Catalog& cat) {
    printHeader("DELETE", "DELETE ENTRY");
    int id = inputInt("Entry ID to delete: ", 1, 99999);
    if (!findEntry(cat, id)) { std::cout << RED << "Entry not found.\n" << RESET; return; }
    if (inputYN("Delete entry #" + std::to_string(id) + "?")) {
        deleteEntry(cat, id);
        std::cout << "\n" << GREEN << BOLD << "Entry deleted." << RESET << "\n\n";
    }
}

// ─── OMDb ────────────────────────────────────────────────────────────────────
static void doOMDb(Catalog& cat) {
    printHeader("OMDb", "FETCH FROM OMDb");

    tabulate::Table info;
    info.add_row({"Field", "Input"});
    info.add_row({"Title", "Enter movie title to search"});
    info.add_row({"Year",  "Enter release year or 0 to skip"});
    info.column(0).format().width(14);
    info.column(1).format().width(34);
    info.row(0).format().font_style({tabulate::FontStyle::bold});
    std::cout << info << "\n\n";

    std::string title = inputLine("Title : ");
    int year = inputInt("Year  : ", 0, 2100);

    std::cout << "\n" << CYAN << "Searching OMDb for \""
              << BOLD << title << RESET << CYAN << "\"..." << RESET << "\n\n";

    auto res = fetchOMDb(title, year);
    if (!res) { std::cout << RED << "No results found.\n" << RESET; return; }

    if (inputYN("Add to catalog?")) {
        Entry e  = *res;
        e.rating = inputFloat("Rating 1-10 : ", 1.0f, 10.0f);
        e.notes  = inputLine("Notes       : ", true);
        e.status = inputYN("Mark as Watched?") ? WatchStatus::Done : WatchStatus::Pending;
        addEntry(cat, e);

        std::cout << "\n" << BOLD << GREEN << "ENTRY SAVED!" << RESET << "\n\n";

        tabulate::Table summary;
        summary.add_row({"Field", "Value"});
        summary.add_row({"ID",     std::to_string(cat.entries.back().id)});
        summary.add_row({"Title",  e.title});
        summary.add_row({"Genre",  e.genre.empty() ? "-" : e.genre});
        summary.add_row({"Year",   e.year > 0 ? std::to_string(e.year) : "-"});
        summary.add_row({"Rating", std::to_string((int)e.rating) + "/10"});
        summary.add_row({"Status", e.status == WatchStatus::Done ? "Watched" : "Not Watched"});
        summary.add_row({"Notes",  e.notes.empty() ? "-" : e.notes});
        summary.column(0).format().width(14);
        summary.column(1).format().width(30);
        summary.row(0).format().font_style({tabulate::FontStyle::bold});
        std::cout << summary << "\n\n";
    }
    std::cout << YELLOW << BOLD << "  Press Enter to go back to main menu..." << RESET;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cout << "\n";
}

// ─── Stats ───────────────────────────────────────────────────────────────────
static void doStats(const Catalog& cat) {
    printHeader(">>", "CATALOG STATISTICS");

    if (cat.entries.empty()) {
        std::cout << RED << "  No entries in catalog yet.\n" << RESET;
        return;
    }

    // ✅ New — requires only 1 enter
    auto pressEnter = [](const std::string& msg = "  Press Enter to continue...") {
    std::cout << YELLOW << msg << RESET;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cout << "\n";
    };

    int total       = (int)cat.entries.size();
    int watched     = 0;
    int movies      = 0;
    float sumRating = 0.0f;
    float maxRating = 0.0f;
    std::string topTitle;

    for (const auto& e : cat.entries) {
        if (e.status == WatchStatus::Done) watched++;
        if (e.type   == MediaType::Movie)  movies++;
        sumRating += e.rating;
        if (e.rating > maxRating) { maxRating = e.rating; topTitle = e.title; }
    }
    float avgRating = sumRating / total;

    // ── 1. Overview ──────────────────────────────────
    std::cout << BOLD << YELLOW << "  1. Overview\n" << RESET;
    printDivider(44);

    tabulate::Table overview;
    overview.add_row({"Stat",           "Value"});
    overview.add_row({"Total Entries",  std::to_string(total)});
    overview.add_row({"Movies",         std::to_string(movies)});
    overview.add_row({"Watched / Read", std::to_string(watched)});
    overview.add_row({"Average Rating", std::to_string(avgRating).substr(0,4) + " / 10"});
    overview.add_row({"Highest Rated",  topTitle + " (" + std::to_string(maxRating).substr(0,3) + ")"});
    overview.column(0).format().width(18);
    overview.column(1).format().width(30);
    overview.row(0).format().font_style({tabulate::FontStyle::bold});
    std::cout << "\n" << overview << "\n\n";
    pressEnter();

    // ── 2. Watch Progress ────────────────────────────
    std::cout << BOLD << YELLOW << "  2. Watch Progress\n" << RESET;
    printDivider(44);

    int pct    = (watched * 100) / total;
    int filled = (pct * 30) / 100;
    std::string bar;
    for (int i = 0; i < filled; i++)  bar += "\xe2\x96\x88";
    for (int i = filled; i < 30; i++) bar += "\xe2\x96\x91";

    std::cout << "\n  " << GREEN << bar   << RESET
              << "  "   << BOLD  << pct   << "%" << RESET
              << "  ("  << GREEN << watched << RESET
              << "/"    << total << " watched)\n\n";
    pressEnter();

    // ── 3. Top 5 Rated ───────────────────────────────
    std::cout << BOLD << YELLOW << "  3. Top 5 Rated\n" << RESET;
    printDivider(44);

    auto sorted = cat.entries;
    std::sort(sorted.begin(), sorted.end(), [](const Entry& a, const Entry& b){
        return a.rating > b.rating;
    });

    tabulate::Table top;
    top.add_row({"Rank", "Title", "Type", "Rating"});
    top.row(0).format().font_style({tabulate::FontStyle::bold});
    top.column(0).format().width(6);
    top.column(1).format().width(26);
    top.column(2).format().width(8);
    top.column(3).format().width(8);

    int limit = std::min((int)sorted.size(), 5);
    for (int i = 0; i < limit; i++) {
        auto& e = sorted[i];
        top.add_row({
            "#" + std::to_string(i + 1),
            e.title,
            e.type == MediaType::Movie ? "Movie" : "Book",
            std::to_string(e.rating).substr(0, 3) + "/10"
        });
    }
    std::cout << "\n" << top << "\n\n";
    pressEnter();

    // ── 4. Rating Distribution ───────────────────────
    std::cout << BOLD << YELLOW << "  4. Rating Distribution\n" << RESET;
    printDivider(44);

    int cnt1=0, cnt2=0, cnt3=0, cnt4=0;
    for (const auto& e : cat.entries) {
        if      (e.rating >= 9.0f) cnt4++;
        else if (e.rating >= 7.0f) cnt3++;
        else if (e.rating >= 5.0f) cnt2++;
        else                       cnt1++;
    }

    auto printBar = [](const std::string& label, int cnt, const std::string& color) {
        std::string chart;
        for (int i = 0; i < cnt * 4; i++) chart += "\xe2\x96\x88";
        std::cout << "  " << color << label << RESET
                  << "  |" << color << chart << RESET
                  << "  " << cnt << "\n";
    };

    std::cout << "\n";
    printBar("1-4  (Low)  ", cnt1, RED);
    printBar("5-6  (Ok)   ", cnt2, YELLOW);
    printBar("7-8  (Good) ", cnt3, CYAN);
    printBar("9-10 (Great)", cnt4, GREEN);
    std::cout << "\n";
    pressEnter("  Press Enter to go back...");
}

// ─── Menu ────────────────────────────────────────────────────────────────────
static void printMenu(const std::string& username) {
    std::cout << "\n";
    std::cout << MAGENTA << BOLD;
    std::cout << "  ███╗   ███╗ █████╗ ██╗███╗   ██╗    ███╗   ███╗███████╗███╗   ██╗██╗   ██╗\n";
    std::cout << "  ████╗ ████║██╔══██╗██║████╗  ██║    ████╗ ████║██╔════╝████╗  ██║██║   ██║\n";
    std::cout << "  ██╔████╔██║███████║██║██╔██╗ ██║    ██╔████╔██║█████╗  ██╔██╗ ██║██║   ██║\n";
    std::cout << "  ██║╚██╔╝██║██╔══██║██║██║╚██╗██║    ██║╚██╔╝██║██╔══╝  ██║╚██╗██║██║   ██║\n";
    std::cout << "  ██║ ╚═╝ ██║██║  ██║██║██║ ╚████║    ██║ ╚═╝ ██║███████╗██║ ╚████║╚██████╔╝\n";
    std::cout << "  ╚═╝     ╚═╝╚═╝  ╚═╝╚═╝╚═╝  ╚═══╝    ╚═╝     ╚═╝╚══════╝╚═╝  ╚═══╝ ╚═════╝\n";
    std::cout << RESET << "\n";

    int menuPad = 10;
    std::string sp = std::string(menuPad, ' ');

    auto hline = [&](char fill) {
        std::cout << MAGENTA << sp << "+"
                  << std::string(8,  fill) << "+"
                  << std::string(14, fill) << "+"
                  << std::string(26, fill) << "+"
                  << RESET << "\n";
    };

    auto row = [&](const std::string& k, const std::string& o,
                   const std::string& d, const std::string& color = "") {
        std::string pk = k + std::string(6  - (int)k.size(), ' ');
        std::string po = o + std::string(12 - (int)o.size(), ' ');
        std::string pd = d + std::string(24 - (int)d.size(), ' ');
        std::cout << MAGENTA << sp << "|" << RESET
                  << " " << color << BOLD << pk << RESET << " "
                  << MAGENTA << "|" << RESET
                  << " " << color << po << RESET << " "
                  << MAGENTA << "|" << RESET
                  << " " << color << pd << RESET << " "
                  << MAGENTA << "|" << RESET << "\n";
    };

    hline('=');
    row("Key", "Option", "Description", WHITE);
    hline('=');
    row("1", "List",   "Browse your catalog",  CYAN);
    hline('-');
    row("2", "View",   "See entry details",    CYAN);
    hline('-');
    row("3", "Add",    "Add new entry",        CYAN);
    hline('-');
    row("4", "Edit",   "Edit an entry",        CYAN);
    hline('-');
    row("5", "Delete", "Remove an entry",      CYAN);
    hline('-');
    row("6", "OMDb",   "Fetch from OMDb",      YELLOW);
    hline('-');
    row("7", "Stats",  "View statistics",      CYAN);
    hline('-');
    row("8", "Logout", "Switch account",       GREEN);
    hline('-');
    row("0", "Quit",   "Exit program",         RED);
    hline('=');

    std::cout << "\n";
}

// ─── Auth Screen ─────────────────────────────────────────────────────────────
static std::string authScreen() {
    while (true) {
        std::cout << "\n\n\n";

        // ── Welcome ASCII centered ─────────────────────
        std::cout << MAGENTA << BOLD;
        printCentered("███╗   ███╗ ██████╗ ██╗   ██╗██╗███████╗      ██████╗ █████╗ ████████╗ █████╗ ██╗      ██████╗  ██████╗");
        printCentered("████╗ ████║██╔═══██╗██║   ██║██║██╔════╝     ██╔════╝██╔══██╗╚══██╔══╝██╔══██╗██║     ██╔═══██╗██╔════╝");
        printCentered("██╔████╔██║██║   ██║██║   ██║██║█████╗       ██║     ███████║   ██║   ███████║██║     ██║   ██║██║  ███╗");
        printCentered("██║╚██╔╝██║██║   ██║╚██╗ ██╔╝██║██╔══╝       ██║     ██╔══██║   ██║   ██╔══██║██║     ██║   ██║██║   ██║");
        printCentered("██║ ╚═╝ ██║╚██████╔╝ ╚████╔╝ ██║███████╗     ╚██████╗██║  ██║   ██║   ██║  ██║███████╗╚██████╔╝╚██████╔╝");
        printCentered("╚═╝     ╚═╝ ╚═════╝   ╚═══╝  ╚═╝╚══════╝      ╚═════╝╚═╝  ╚═╝   ╚═╝   ╚═╝  ╚═╝╚══════╝ ╚═════╝  ╚═════╝");
        std::cout << RESET << "\n";


        // ── Auth menu centered ────────────────────────
        int pad = 50;
        std::string sp = std::string(pad, ' ');

        std::cout << MAGENTA;
        std::cout << sp << "╭───────────────────────────────╮\n";
        std::cout << sp << "│             Option            │\n";
        std::cout << sp << "├────────┬──────────────────────┤\n";
        std::cout << MAGENTA;
        std::cout << sp << "│   1    │        Login         │\n";
        std::cout << sp << "├────────┼──────────────────────┤\n";
        std::cout << sp << "│   2    │       Register       │\n";
        std::cout << MAGENTA;
        std::cout << sp << "├────────┼──────────────────────┤\n";
        std::cout << MAGENTA;
        std::cout << sp << "│   0    │         Quit         │\n";
        std::cout << MAGENTA;
        std::cout << sp << "╰────────┴──────────────────────╯\n";
        std::cout << "\n";

        std::cout << sp << ">> Choose: ";
        std::string choiceStr;
        std::getline(std::cin, choiceStr);
        int choice = -1;
        try { choice = std::stoi(choiceStr); } catch (...) { choice = -1; }
        if (choice < 0 || choice > 2) {
        std::cout << "\n" << sp << RED << BOLD
              << "[\xe2\x9c\x96] Invalid option! Please choose 0, 1, or 2." << RESET << "\n\n";
    continue;
}

if (choice == 0) {
    printGoodbye();
    exit(0);
}
        std::cout << "\n";

        // ── Login / Register form centered ────────────
        std::string formTitle = (choice == 1) ? "You choose login" : "You choose register";
        std::cout << sp << BOLD << CYAN << "── " << formTitle << " ──" << RESET << "\n\n";

        std::cout << GREEN;
        std::cout << sp << "╭────────────────┬────────────────────────────╮\n";
        std::cout << GREEN;
        std::cout << sp << "│ Username       │  Enter your username       │\n";
        std::cout << sp << "├────────────────┼────────────────────────────┤\n";
        std::cout << sp << "│ Password       │  Enter your password       │\n";
        std::cout << GREEN;
        std::cout << sp << "╰────────────────┴────────────────────────────╯\n\n";

        std::cout << sp << "Username : ";
        std::string user;
        std::getline(std::cin, user);
        std::cout << sp << "Password : ";
        std::string pass;
        std::getline(std::cin, pass);

        if (choice == 1) {
            auto res = loginUser(user, pass);
            if (res) {
                printLoading("Logging in");
                std::cout << "\n" << sp << BOLD << GREEN
                    << "[\xe2\x9c\x94] Login successful! Welcome back, "
                    << CYAN << user << GREEN << "!" << RESET << "\n\n";
                return *res;
}
            std::cout << "\n" << sp << RED << BOLD
                      << "[\xe2\x9c\x96] Invalid username or password." << RESET << "\n\n";
                        std::cout << sp << YELLOW << "Press Enter to continue..." << RESET;
                        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                        std::cout << "\n";
        } else {
            auto res = registerUser(user, pass);
            if (res) {
                printLoading("Setting up account");
                std::cout << "\n" << sp << BOLD << GREEN
                          << "[\xe2\x9c\x94] Account created! Welcome, "
                          << CYAN << user << GREEN << "!" << RESET << "\n\n";
                return *res;
            }           
            std::cout << "\n" << sp << RED << BOLD
                      << "[!!] Username already taken." << RESET << "\n\n";
                        std::cout << sp << YELLOW << "Press Enter to continue..." << RESET;
                        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                        std::cout << "\n";
        }
    }
}

// ─── Main ────────────────────────────────────────────────────────────────────
int main() {
#ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    printTitle();

    std::cout << YELLOW << "\n" << RESET;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cout << "\n";

    while (true) {
        std::string username = authScreen();
        Catalog cat = loadCatalog(username);
        bool running = true;

        while (running) {
            printMenu(username);
            std::cout << CYAN << BOLD << "  >> " << RESET;
            std::string choiceStr;
            std::getline(std::cin, choiceStr);
            int choice = -1;
            try { choice = std::stoi(choiceStr); } catch (...) {}
            if (choice < 0 || choice > 8) {
                std::cout << RED << BOLD << "  Please enter 0-8.\n" << RESET;
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                continue;
            }
            switch (choice) {
                case 1: doList(cat);      break;
                case 2: doView(cat);      break;
                case 3: doAdd(cat);       break;
                case 4: doEdit(cat);      break;
                case 5: doDelete(cat);    break;
                case 6: doOMDb(cat);      break;
                case 7: doStats(cat);     break;
                case 8: running = false;  break;
                case 0: printGoodbye(); return 0;
            }
        }
    }
}