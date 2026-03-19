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
#include <conio.h> 
#include <windows.h>
#else
#include <termios.h>   
#include <unistd.h>
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

static int getTermWidth() {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    return csbi.srWindow.Right - csbi.srWindow.Left + 1;
}

static std::string centerPad(const std::string& text, int termWidth = -1) {
    if (termWidth < 0) termWidth = getTermWidth();
    int pad = (termWidth - (int)text.size()) / 2;
    if (pad < 0) pad = 0;
    return std::string(pad, ' ') + text;
}

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

static void printLoading(const std::string& msg = "Loading") {
    int termWidth = getTermWidth();
    int barWidth  = 40;
    int pad = (termWidth - barWidth - 12) / 2;
    if (pad < 0) pad = 0;
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

// ✅ AFTER — use getTermWidth()
static void printCentered(const std::string& text) {
    int termWidth = getTermWidth();
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

// ✅ AFTER — use getTermWidth()
static void printTitle() {
    auto center = [&](const std::string& s) {
        int pad = (getTermWidth() - (int)s.size()) / 2;
        if (pad < 0) pad = 0;
        std::cout << std::string(pad, ' ') << s << "\n";
    };

    std::cout << GREEN << BOLD;
    center("################################################################################");
    center("#                                                                              #");
    center("#                           MEDIA CATALOG MANAGER                              #");
    center("#                                                                              #");
    center("#                        Your personal movie tracker                           #");
    center("#                                                                              #");
    center("################################################################################");
    std::cout << RESET << "\n";
    std::cout << GREEN;
    int pad = (getTermWidth() - 28) / 2;         // ✅ dynamic — 28 = length of the text
    if (pad < 0) pad = 0;
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
    int id = inputInt(centerPad("Entry ID: "), 1, 99999);
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
    e.title  = inputLine(centerPad("Title   : "));
    int t    = inputInt (centerPad("Type  1=Movie  2=Book : "), 1, 2);
    e.genre  = inputLine(centerPad("Genre   : "), true);
    e.year   = inputInt (centerPad("Year    : "), 1888, 2100);
    e.rating = inputFloat(centerPad("Rating  : "), 1.0f, 10.0f);
    e.notes  = inputLine(centerPad("Notes   : "), true);
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
    int id = inputInt(centerPad("Entry ID to edit: "), 1, 99999);
    Entry* ep = findEntry(cat, id);
    if (!ep) { std::cout << RED << "Entry not found.\n" << RESET; return; }
    printEntry(*ep);
    std::cout << YELLOW << "Press Enter to keep existing value.\n\n" << RESET;
    Entry updated = *ep;
    std::string s;

    // ✅ get terminal width once
    int tw = getTermWidth();

    // ✅ lambda — pads a label to be centered based on terminal width
    // 20 = estimated length of the value shown after the label e.g. "[somevalue]: "
    auto cp = [&](const std::string& label) {
        int p = (tw - (int)label.size() - 20) / 2;
        if (p < 0) p = 0;
        return std::string(p, ' ') + label;
    };

    // BEFORE:  std::cout << "Title  [" << CYAN << updated.title  << RESET << "]: ";
    // ✅ AFTER:
    std::cout << cp("Title  [") << CYAN << updated.title  << RESET << "]: ";
    std::getline(std::cin, s); if (!s.empty()) updated.title = s;

    // BEFORE:  std::cout << "Genre  [" << CYAN << updated.genre  << RESET << "]: ";
    // ✅ AFTER:
    std::cout << cp("Genre  [") << CYAN << updated.genre  << RESET << "]: ";
    std::getline(std::cin, s); if (!s.empty()) updated.genre = s;

    // BEFORE:  std::cout << "Year   [" << CYAN << updated.year   << RESET << "]: ";
    // ✅ AFTER:
    std::cout << cp("Year   [") << CYAN << updated.year   << RESET << "]: ";
    std::getline(std::cin, s);
    if (!s.empty()) try { updated.year = std::stoi(s); } catch (...) {}

    // BEFORE:  std::cout << "Rating [" << CYAN << updated.rating << RESET << "]: ";
    // ✅ AFTER:
    std::cout << cp("Rating [") << CYAN << updated.rating << RESET << "]: ";
    std::getline(std::cin, s);
    if (!s.empty()) try {
        float v = std::stof(s);
        if (v >= 0 && v <= 10) updated.rating = v;
    } catch (...) {}

    // BEFORE:  std::cout << "Notes  [" << CYAN << updated.notes  << RESET << "]: ";
    // ✅ AFTER:
    std::cout << cp("Notes  [") << CYAN << updated.notes  << RESET << "]: ";
    std::getline(std::cin, s); if (!s.empty()) updated.notes = s;

    // ✅ Status line — same cp() centering
    std::string currentStatus = updated.status == WatchStatus::Done
        ? (updated.type == MediaType::Movie ? "Watched" : "Read")
        : (updated.type == MediaType::Movie ? "Not Watched" : "Want to Read");

    // BEFORE:  std::cout << "Status [" << CYAN << currentStatus << RESET << "] Change? ";
    // ✅ AFTER:
    std::cout << cp("Status [") << CYAN << currentStatus << RESET << "] Change? ";
    if (inputYN(""))
        updated.status = inputYN(updated.type == MediaType::Movie
            ? "Mark as Watched?" : "Mark as Read?")
            ? WatchStatus::Done : WatchStatus::Pending;

    if (editEntry(cat, id, updated))
        std::cout << "\n" << GREEN << BOLD << "Entry updated!" << RESET << "\n\n";
    else
        std::cout << "\n" << RED << "Update failed.\n" << RESET;
}

// ─── Delete ──────────────────────────────────────────────────────────────────
static void doDelete(Catalog& cat) {
    printHeader("DELETE", "DELETE ENTRY");
    int id = inputInt(centerPad("Entry ID to delete: "), 1, 99999);
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

    std::string title = inputLine(centerPad("Title : "));
    int year = inputInt(centerPad("Year  : "), 0, 2100);

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
    while (_getch() != '\r') {}
    system("cls");
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
        while (_getch() != '\r') {}
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
    float avgRating = total > 0 ? sumRating / total : 0.0f;

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
    std::cout << YELLOW << "  Press Enter to go back to main menu..." << RESET;
    while (_getch() != '\r') {}
    system("cls");
}

// ─── Admin ───────────────────────────────────────────────────────────────────
static void adminViewUsers() {
    printHeader(">>", "ALL USERS");
    auto users = getAllUsers();
    if (users.empty()) {
        std::cout << RED << "  No users found.\n" << RESET;
        return;
    }

    int menuPad = 10;
    std::string sp = std::string(menuPad, ' ');

    auto hline = [&](char fill) {
        std::cout << MAGENTA << sp << "+"
                  << std::string(6,  fill) << "+"
                  << std::string(20, fill) << "+"
                  << RESET << "\n";
    };

    auto row = [&](const std::string& no, const std::string& name) {
        std::string pno   = no   + std::string(4  - (int)no.size(),   ' ');
        std::string pname = name + std::string(18 - (int)name.size(), ' ');
        std::cout << MAGENTA << sp << "|" << RESET
                  << " " << CYAN << pno   << RESET << " "
                  << MAGENTA << "|" << RESET
                  << " " << CYAN << pname << RESET << " "
                  << MAGENTA << "|" << RESET << "\n";
    };

    hline('=');
    row("No", "Username");
    hline('=');
    int displayNum = 1;
    for (int i = 0; i < (int)users.size(); i++) {
        if (users[i] == "admin") continue;           // ✅ skip admin
        row(std::to_string(displayNum++), users[i]);
        hline('-');
    }

    int realCount = 0;
    for (auto& u : users)
        if (u != "admin") realCount++;
    std::cout << "\n" << YELLOW << BOLD << "  Total users: " << realCount << RESET << "\n\n";

    std::cout << YELLOW << BOLD << "  Press Enter to go back..." << RESET;
    while (_getch() != '\r') {}
    system("cls");
}

static void adminViewAllCatalogs() {
    printHeader(">>", "ALL CATALOGS");
    auto users = getAllUsers();
    if (users.empty()) {
        std::cout << RED << "  No users found.\n" << RESET;
        return;
    }

    for (const auto& user : users) {
        if (user == "admin") continue;
        Catalog cat = loadCatalog(user);
        std::cout << BOLD << YELLOW << "  User: " << CYAN << user
                  << YELLOW << " (" << cat.entries.size() << " entries)"
                  << RESET << "\n\n";
        if (cat.entries.empty()) {
            std::cout << DIM << "  No entries.\n\n" << RESET;
        } else {
            printTable(cat.entries, user);
        }
        std::cout << "\n";
    }

    std::cout << YELLOW << BOLD << "  Press Enter to go back..." << RESET;
    while (_getch() != '\r') {}
    system("cls");
}

static void adminResetPassword() {
    printHeader(">>", "RESET USER PASSWORD");

    auto users = getAllUsers();
    if (users.empty()) {
        std::cout << RED << "  No users found.\n" << RESET;
        return;
    }

    std::cout << CYAN << "  Available users:\n" << RESET;
    for (int i = 0; i < (int)users.size(); i++) {
        if (users[i] == "admin") continue;
        std::cout << "  " << CYAN << i + 1 << ". " << users[i] << RESET << "\n";
    }
    std::cout << "\n";

    std::string username = inputLine(centerPad("Enter username to reset : "));
    auto it = std::find(users.begin(), users.end(), username);
    if (it == users.end()) {
        std::cout << RED << BOLD << "  User not found.\n" << RESET;
        return;
    }

    std::string newPass  = inputLine(centerPad("Enter new password      : "));
    if (resetPassword(username, newPass)) {
        std::cout << "\n" << GREEN << BOLD
                  << "  [\xe2\x9c\x94] Password reset successfully for " << username
                  << RESET << "\n\n";
    } else {
        std::cout << RED << BOLD << "  [\xe2\x9c\x96] Failed to reset password.\n" << RESET;
    }

    std::cout << YELLOW << BOLD << "  Press Enter to go back..." << RESET;
    while (_getch() != '\r') {}
    system("cls");
}
static void adminDeleteUser() {
    printHeader(">>", "DELETE USER ACCOUNT");

    auto users = getAllUsers();
    if (users.empty()) {
        std::cout << RED << "  No users found.\n" << RESET;
        return;
    }

    std::cout << CYAN << "  Available users:\n" << RESET;
    for (int i = 0; i < (int)users.size(); i++) {
        if (users[i] == "admin") continue;
        std::cout << "  " << CYAN << i + 1 << ". " << users[i] << RESET << "\n";
    }
    std::cout << "\n";

    std::string username = inputLine(centerPad("Enter username to delete : "));

    if (username == "admin") {
        std::cout << RED << BOLD << "  Cannot delete admin account.\n" << RESET;
        return;
    }

    auto it = std::find(users.begin(), users.end(), username);
    if (it == users.end()) {
        std::cout << RED << BOLD << "  User not found.\n" << RESET;
        return;
    }

    if (inputYN("  Are you sure you want to delete " + username + "?")) {
        if (deleteUser(username)) {
            std::cout << "\n" << GREEN << BOLD
                      << "  [\xe2\x9c\x94] User " << username << " deleted successfully."
                      << RESET << "\n\n";
        } else {
            std::cout << RED << BOLD << "  [\xe2\x9c\x96] Failed to delete user.\n" << RESET;
        }
    } else {
        std::cout << YELLOW << "  Cancelled.\n" << RESET;
    }

    std::cout << YELLOW << BOLD << "  Press Enter to go back..." << RESET;
    while (_getch() != '\r') {}
    system("cls");
}

static void printAdminMenu() {
    std::cout << "\n";

    // ── ASCII title centered ──────────────────────
    int termW = getTermWidth();
    int asciiW = 44;  // actual width of ADMIN ASCII art
    int asciiPad = (termW - asciiW) / 2;
    if (asciiPad < 0) asciiPad = 0;
    std::string ap = std::string(asciiPad, ' ');

    std::cout << RED << BOLD;
    std::cout << ap << " █████╗ ██████╗ ███╗   ███╗██╗███╗   ██╗\n";
    std::cout << ap << "██╔══██╗██╔══██╗████╗ ████║██║████╗  ██║\n";
    std::cout << ap << "███████║██║  ██║██╔████╔██║██║██╔██╗ ██║\n";
    std::cout << ap << "██╔══██║██║  ██║██║╚██╔╝██║██║██║╚██╗██║\n";
    std::cout << ap << "██║  ██║██████╔╝██║ ╚═╝ ██║██║██║ ╚████║\n";
    std::cout << ap << "╚═╝  ╚═╝╚═════╝ ╚═╝     ╚═╝╚═╝╚═╝  ╚═══╝\n";
    std::cout << RESET << "\n";

    // ── Table centered — table width = 8+30+4 borders = 44 ──
    int tableW = 44;
    int menuPad = (termW - tableW) / 2;            // ✅ dynamic
    if (menuPad < 0) menuPad = 0;
    std::string sp = std::string(menuPad, ' ');

    auto hline = [&](char fill) {
        std::cout << MAGENTA << sp << "+"
                  << std::string(8,  fill) << "+"
                  << std::string(30, fill) << "+"
                  << RESET << "\n";
    };

    auto row = [&](const std::string& k, const std::string& o,
                   const std::string& color = "") {
        std::string pk = k + std::string(6  - (int)k.size(), ' ');
        std::string po = o + std::string(28 - (int)o.size(), ' ');
        std::cout << MAGENTA << sp << "|" << RESET
                  << " " << color << BOLD << pk << RESET << " "
                  << MAGENTA << "|" << RESET
                  << " " << color << po << RESET << " "
                  << MAGENTA << "|" << RESET << "\n";
    };

    hline('=');
    row("Key", "Option", WHITE);
    hline('=');
    row("1", "View all users",         CYAN);
    hline('-');
    row("2", "View all catalogs",      CYAN);
    hline('-');
    row("3", "Reset user password",    YELLOW);
    hline('-');
    row("4", "Delete user account",    RED);
    hline('-');
    row("0", "Logout",                 RED);
    hline('=');

    std::cout << "\n";
}

// ─── Menu ────────────────────────────────────────────────────────────────────
// ✅ AFTER
static void printMenu(const std::string& username) {
    std::cout << "\n";

    // ── ASCII title centered ──────────────────────
    int termW = getTermWidth();
    int asciiW = 78;  // actual width of the ASCII art lines above
    int asciiPad = (termW - asciiW) / 2;
    if (asciiPad < 0) asciiPad = 0;
    std::string ap = std::string(asciiPad, ' ');

    std::cout << MAGENTA << BOLD;
    std::cout << ap << "███╗   ███╗ █████╗ ██╗███╗   ██╗    ███╗   ███╗███████╗███╗   ██╗██╗   ██╗\n";
    std::cout << ap << "████╗ ████║██╔══██╗██║████╗  ██║    ████╗ ████║██╔════╝████╗  ██║██║   ██║\n";
    std::cout << ap << "██╔████╔██║███████║██║██╔██╗ ██║    ██╔████╔██║█████╗  ██╔██╗ ██║██║   ██║\n";
    std::cout << ap << "██║╚██╔╝██║██╔══██║██║██║╚██╗██║    ██║╚██╔╝██║██╔══╝  ██║╚██╗██║██║   ██║\n";
    std::cout << ap << "██║ ╚═╝ ██║██║  ██║██║██║ ╚████║    ██║ ╚═╝ ██║███████╗██║ ╚████║╚██████╔╝\n";
    std::cout << ap << "╚═╝     ╚═╝╚═╝  ╚═╝╚═╝╚═╝  ╚═══╝    ╚═╝     ╚═╝╚══════╝╚═╝  ╚═══╝ ╚═════╝\n";
    std::cout << RESET << "\n";

    // ── Table centered — table width = 8+14+26+4 borders = 56 ──
    int tableW = 56;
    int menuPad = (termW - tableW) / 2;            // ✅ dynamic
    if (menuPad < 0) menuPad = 0;
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
        int termW = getTermWidth();
        auto cl = [&](const std::string& s, int visualW) {
            int p = (termW - visualW) / 2;
            if (p < 0) p = 0;
            std::cout << std::string(p, ' ') << s << "\n";
        };

       std::cout << MAGENTA << BOLD;
        // ── MOVIE ────────────────────────────────────
        cl("███╗   ███╗ ██████╗ ██╗   ██╗██╗███████╗", 41);
        cl("████╗ ████║██╔═══██╗██║   ██║██║██╔════╝", 41);
        cl("██╔████╔██║██║   ██║██║   ██║██║█████╗  ", 41);
        cl("██║╚██╔╝██║██║   ██║╚██╗ ██╔╝██║██╔══╝  ", 41);
        cl("██║ ╚═╝ ██║╚██████╔╝ ╚████╔╝ ██║███████╗", 41);
        cl("╚═╝     ╚═╝ ╚═════╝   ╚═══╝  ╚═╝╚══════╝", 41);
        std::cout << "\n";

        // ── CATALOG ──────────────────────────────────
        std::cout << MAGENTA << BOLD;
        cl(" ██████╗ █████╗ ████████╗ █████╗ ██╗      ██████╗  ██████╗ ", 61);
        cl("██╔════╝██╔══██╗╚══██╔══╝██╔══██╗██║     ██╔═══██╗██╔════╝ ", 61);
        cl("██║     ███████║   ██║   ███████║██║     ██║   ██║██║  ███╗ ", 61);
        cl("██║     ██╔══██║   ██║   ██╔══██║██║     ██║   ██║██║   ██║ ", 61);
        cl("╚██████╗██║  ██║   ██║   ██║  ██║███████╗╚██████╔╝╚██████╔╝ ", 61);
        cl(" ╚═════╝╚═╝  ╚═╝   ╚═╝   ╚═╝  ╚═╝╚══════╝ ╚═════╝  ╚═════╝ ", 61);
        std::cout << RESET << "\n";


        // ── Auth menu centered ────────────────────────
        // int termW = getTermWidth();
        int pad = (termW - 31) / 2;  // 31 = width of the box
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
        std::string pass = "";
        char ch;
        while ((ch = _getch()) != '\r') {    // '\r' = Enter on Windows
            if (ch == '\b' && !pass.empty()) {
                pass.pop_back();
                std::cout << "\b \b" << std::flush;  // erase the * on screen
            } else if (ch != '\b') {
                pass += ch;
                std::cout << '*' << std::flush;      // show * instead of character
            }
        }
        std::cout << "\n";

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
                        while (_getch() != '\r') {}
                        system("cls");
        } else {
            auto res = registerUser(user, pass);
            if (res) {
                printLoading("Setting up account");
                std::cout << "\n" << sp << BOLD << GREEN
                          << "[\xe2\x9c\x94] Account created! Welcome, "
                          << CYAN << user << GREEN << "!" << RESET << "\n\n";
                return *res;
            }
            // ✅ show correct reason
            if (user.empty() || pass.empty())
                std::cout << "\n" << sp << RED << BOLD
                          << "[!!] Username and password cannot be empty." << RESET << "\n\n";
            else if (user == "public")
                std::cout << "\n" << sp << RED << BOLD
                          << "[!!] That username is reserved." << RESET << "\n\n";
            else
                std::cout << "\n" << sp << RED << BOLD
                          << "[!!] Username already taken." << RESET << "\n\n";
            std::cout << sp << YELLOW << "Press Enter to continue..." << RESET;
             while (_getch() != '\r') {}
            system("cls");
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
    registerUser("admin", "admin123");
    printTitle();

    std::cout << centerPad("Press Enter to continue...") << YELLOW << BOLD;
    std::cout << RESET;
    while (_getch() != '\r') {}
    system("cls");
    while (true) {
        std::string username = authScreen();
        bool running = true;

    if (username == "admin") {
        std::cout << "\n" << centerPad("Press Enter to access Admin Panel...") << YELLOW << BOLD;
        std::cout << RESET;
        while (_getch() != '\r') {}
        system("cls");                          // ✅ clears login screen
        while (running) {
            printAdminMenu();
            std::cout << centerPad(">> Choice: ") << CYAN << BOLD;
            std::cout << RESET;
            std::string choiceStr;
            std::getline(std::cin, choiceStr);
            int choice = -1;
            try { choice = std::stoi(choiceStr); } catch (...) {}
            if (choice < 0 || choice > 4) {
                std::cout << RED << BOLD << "  Please enter 0-4.\n" << RESET;
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                continue;
            }
            switch (choice) {
                case 1: adminViewUsers();       break;
                case 2: adminViewAllCatalogs(); break;
                case 3: adminResetPassword();   break;
                case 4: adminDeleteUser();      break;
                case 0:
                    system("cls");              // ✅ clear screen on logout
                    running = false;
                    break;
            }
        }
    } else {
        std::cout << "\n" << YELLOW << BOLD
                  << "  Press Enter to continue..." << RESET;
        while (_getch() != '\r') {}
        system("cls");                          // ✅ clears login screen
        Catalog cat = loadCatalog(username);
        while (running) {
            printMenu(username);
            std::cout << centerPad(">> Choice: ") << CYAN << BOLD;
            std::cout << RESET;
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
                case 8:
                    system("cls");              // ✅ clear screen on logout
                    running = false;
                    break;
                case 0: printGoodbye(); return 0;
            }
        }
    }
}
}