#include "catalog.hpp"
#include "storage.hpp"
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <cctype>
#include <nlohmann/json.hpp>      
#include <tabulate/tabulate.hpp>  
           

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
    std::cout << "\n";
    std::cout << "  Catalog: " << label << " (" << entries.size() << " entries)\n\n";

    tabulate::Table table;

    table.add_row({"ID", "Title", "Type", "Genre", "Year", "Rating", "Status"});

    for (const auto& e : entries) {
        std::string rat = e.rating > 0 ? std::to_string((int)e.rating) + "/10" : "-";
        std::string yr  = e.year   > 0 ? std::to_string(e.year) : "-";
        table.add_row({
            std::to_string(e.id),
            e.title,
            e.type == MediaType::Movie ? "Movie" : "Book",
            e.genre,
            yr,
            rat,
            statusStr(e)
        });
    }

    table.column(0).format().width(5);
    table.column(1).format().width(28);
    table.column(2).format().width(7);
    table.column(3).format().width(14);
    table.column(4).format().width(6);
    table.column(5).format().width(8);
    table.column(6).format().width(14);

    table.row(0).format()
        .font_style({tabulate::FontStyle::bold});

    std::cout << table << "\n\n";
}

void printEntry(const Entry& e) {
    std::string rat = e.rating > 0 ? std::to_string((int)e.rating) + "/10" : "-";
    std::string yr  = e.year   > 0 ? std::to_string(e.year) : "-";

    tabulate::Table table;

    table.add_row({"ID", "Title", "Type", "Genre", "Year", "Rating", "Status", "Director", "IMDb", "Plot", "Notes"});
    table.add_row({
        std::to_string(e.id),
        e.title,
        e.type == MediaType::Movie ? "Movie" : "Book",
        e.genre.empty()      ? "-" : e.genre,
        yr,
        rat,
        statusStr(e),
        e.director.empty()   ? "-" : e.director,
        e.imdbRating.empty() ? "-" : e.imdbRating,
        e.plot.empty()       ? "-" : e.plot,
        e.notes.empty()      ? "-" : e.notes
    });

    table.column(0).format().width(5);
    table.column(1).format().width(20);
    table.column(2).format().width(7);
    table.column(3).format().width(12);
    table.column(4).format().width(6);
    table.column(5).format().width(8);
    table.column(6).format().width(14);
    table.column(7).format().width(20);
    table.column(8).format().width(6);
    table.column(9).format().width(30);
    table.column(10).format().width(20);

    table.row(0).format().font_style({tabulate::FontStyle::bold});

    std::cout << "\n" << table << "\n\n";
}
