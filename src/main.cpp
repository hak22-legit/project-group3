#include "types.hpp"
#include "auth.hpp"
#include "storage.hpp"
#include "catalog.hpp"
#include "input.hpp"
#include "omdb.hpp"
#include <iostream>
#include <algorithm>
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

static void doList(const Catalog& cat) {
    printHeader("📋", "CATALOG LIST");
    printTable(queryEntries(cat, askSort(), {}), cat.username);
}

static void doSearch(const Catalog& cat) {
    printHeader("🔍", "SEARCH CATALOG");
    std::cout << DIM << "Leave blank to skip a filter.\n\n" << RESET;
    FilterOptions opts;
    opts.keyword   = inputLine("Title keyword : ", true);
    opts.genre     = inputLine("Genre         : ", true);
    opts.minRating = inputFloat("Min rating    : ", 0.0f, 10.0f);
    printTable(queryEntries(cat, askSort(), opts), cat.username + " (filtered)");
}

static void doView(Catalog& cat) {
    printHeader("VIEW", "VIEW ENTRY");
    int id = inputInt("Entry ID: ", 1, 99999);
    Entry* ep = nullptr;
    for (auto& e : cat.entries)
        if (e.id == id) { ep = &e; break; }
    if (!ep) {
        std::cout << RED << "Entry not found.\n" << RESET;
        return;
    }
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

static void doDelete(Catalog& cat) {
    printHeader("DELETE", "DELETE ENTRY");
    int id = inputInt("Entry ID to delete: ", 1, 99999);
    if (!findEntry(cat, id)) { std::cout << RED << "Entry not found.\n" << RESET; return; }
    if (inputYN("Delete entry #" + std::to_string(id) + "?")) {
        deleteEntry(cat, id);
        std::cout << "\n" << GREEN << BOLD << "Entry deleted." << RESET << "\n\n";
    }
}

static void doToggle(Catalog& cat) {
    int id = inputInt("Entry ID to toggle status: ", 1, 99999);
    if (toggleStatus(cat, id)) std::cout << GREEN << "Status updated.\n" << RESET;
    else                        std::cout << RED   << "Entry not found.\n" << RESET;
}

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

    std::cout << BOLD << GREEN << "RESULT FOUND" << RESET << "\n\n";

    tabulate::Table details;
    details.add_row({"Field", "Value"});
    details.add_row({"Title",    res->title});
    details.add_row({"Type",     "Movie"});
    details.add_row({"Genre",    res->genre.empty()      ? "-" : res->genre});
    details.add_row({"Year",     res->year > 0 ? std::to_string(res->year) : "-"});
    details.add_row({"Director", res->director.empty()   ? "-" : res->director});
    details.add_row({"IMDb",     res->imdbRating.empty() ? "-" : res->imdbRating});
    details.add_row({"Plot",     res->plot.empty()       ? "-" : res->plot});
    details.column(0).format().width(14);
    details.column(1).format().width(50);
    details.row(0).format().font_style({tabulate::FontStyle::bold});
    std::cout << details << "\n\n";

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
}

// ─── Main Menu ───────────────────────────────────────────────────────────────
static void printMenu(const std::string& username) {
    std::cout << "\n";
    std::cout << BOLD << BG_BLUE << WHITE
              << "  MEDIA CATALOG MANAGER  "
              << RESET << "\n";
    std::cout << CYAN << "  Logged in as: " << BOLD << WHITE << username << RESET << "\n\n";

    tabulate::Table menu;
    menu.add_row({"Key", "Option", "Description"});
    menu.add_row({"1", "List",   "Browse your catalog"});
    menu.add_row({"2", "View",   "See entry details"});
    menu.add_row({"3", "Add",    "Add new entry"});
    menu.add_row({"4", "Edit",   "Edit an entry"});
    menu.add_row({"5", "Delete", "Remove an entry"});
    menu.add_row({"6", "OMDb",   "Fetch from OMDb"});
    menu.add_row({"7", "Logout", "Switch account"});
    menu.add_row({"0", "Quit",   "Exit program"});

    menu.column(0).format().width(6);
    menu.column(1).format().width(12);
    menu.column(2).format().width(24);
    menu.row(0).format()
        .font_style({tabulate::FontStyle::bold})
        .font_color(tabulate::Color::yellow);

    // Key colors
    menu.row(1).cell(0).format().font_color(tabulate::Color::cyan);
    menu.row(2).cell(0).format().font_color(tabulate::Color::cyan);
    menu.row(3).cell(0).format().font_color(tabulate::Color::cyan);
    menu.row(4).cell(0).format().font_color(tabulate::Color::cyan);
    menu.row(5).cell(0).format().font_color(tabulate::Color::cyan);
    menu.row(6).cell(0).format().font_color(tabulate::Color::magenta);
    menu.row(7).cell(0).format().font_color(tabulate::Color::green);
    menu.row(8).cell(0).format().font_color(tabulate::Color::red);

    // Option colors
    menu.row(1).cell(1).format().font_color(tabulate::Color::cyan);
    menu.row(2).cell(1).format().font_color(tabulate::Color::cyan);
    menu.row(3).cell(1).format().font_color(tabulate::Color::cyan);
    menu.row(4).cell(1).format().font_color(tabulate::Color::cyan);
    menu.row(5).cell(1).format().font_color(tabulate::Color::cyan);
    menu.row(6).cell(1).format().font_color(tabulate::Color::magenta);
    menu.row(7).cell(1).format().font_color(tabulate::Color::green);
    menu.row(8).cell(1).format().font_color(tabulate::Color::red);

    // Description colors
    menu.row(1).cell(2).format().font_color(tabulate::Color::white);
    menu.row(2).cell(2).format().font_color(tabulate::Color::white);
    menu.row(3).cell(2).format().font_color(tabulate::Color::white);
    menu.row(4).cell(2).format().font_color(tabulate::Color::white);
    menu.row(5).cell(2).format().font_color(tabulate::Color::white);
    menu.row(6).cell(2).format().font_color(tabulate::Color::white);
    menu.row(7).cell(2).format().font_color(tabulate::Color::white);
    menu.row(8).cell(2).format().font_color(tabulate::Color::white);

    std::cout << menu << "\n\n";
}

// ─── Auth Screen (no tabulate, emojis + ANSI colors) ─────────────────────────
static std::string authScreen() {
    while (true) {
        std::cout << "\n";
        printDivider(44);
        std::cout << BOLD << BG_BLUE << YELLOW
                  << "   🗂️  MEDIA CATALOG MANAGER   "
                  << RESET << "\n";
        std::cout << BG_BLUE << WHITE
                  << "   📽️  Your personal movie & book tracker   "
                  << RESET << "\n";
        printDivider(44);
        std::cout << "\n";

        // Menu options — plain colored lines, no tabulate
        std::cout << BOLD << YELLOW << "  [ Key ]  Option\n" << RESET;
        printDivider(24);
        std::cout << "  [" << CYAN  << BOLD << "  1  " << RESET << "]  " << CYAN  << BOLD << "🔑 Login"    << RESET << "\n";
        std::cout << "  [" << CYAN  << BOLD << "  2  " << RESET << "]  " << CYAN  << BOLD << "📝 Register" << RESET << "\n";
        std::cout << "  [" << RED   << BOLD << "  0  " << RESET << "]  " << RED   << BOLD << "❌ Quit"     << RESET << "\n";
        printDivider(24);
        std::cout << "\n";

        int choice = inputInt("Choice: ", 0, 2);
        if (choice == 0) { std::cout << "\n" << RED << BOLD << "Goodbye! ❌" << RESET << "\n"; exit(0); }

        std::cout << "\n";

        // Login form — plain colored lines, no tabulate
        std::cout << BOLD << YELLOW << "  Field          Input\n" << RESET;
        printDivider(44);
        std::cout << "  " << MAGENTA << BOLD << "👤 Username  " << RESET << WHITE << "  Enter your username\n" << RESET;
        std::cout << "  " << MAGENTA << BOLD << "🔒 Password  " << RESET << WHITE << "  Enter your password\n" << RESET;
        printDivider(44);
        std::cout << "\n";

        std::string user = inputLine("👤 Username : ");
        std::string pass = inputLine("🔒 Password : ");

        if (choice == 1) {
            auto res = loginUser(user, pass);
            if (res) {
                std::cout << "\n✅ " << BOLD << GREEN
                          << "Login successful! Welcome back, "
                          << CYAN << user << GREEN << "!"
                          << RESET << "\n\n";
                return *res;
            }
            std::cout << "\n❌ " << RED << BOLD
                      << "Invalid username or password. Try again."
                      << RESET << "\n\n";
        } else {
            auto res = registerUser(user, pass);
            if (res) {
                std::cout << "\n✅ " << BOLD << GREEN
                          << "Account created! Welcome, "
                          << CYAN << user << GREEN << "!"
                          << RESET << "\n\n";
                return *res;
            }
            std::cout << "\n❌ " << RED << BOLD
                      << "Username already taken or invalid. Try again."
                      << RESET << "\n\n";
        }
    }
}

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

    while (true) {
        std::string username = authScreen();
        Catalog cat = loadCatalog(username);
        bool running = true;

        while (running) {
            printMenu(username);
            int choice = inputInt("Choice: ", 0, 7);
            switch (choice) {
                case 1: doList(cat);         break;
                case 2: doView(cat);         break;
                case 3: doAdd(cat);          break;
                case 4: doEdit(cat);         break;
                case 5: doDelete(cat);       break;
                case 6: doOMDb(cat);         break;
                case 7: running = false;     break;
                case 0: std::cout << "\nGoodbye!\n"; return 0;
            }
        }
    }
}