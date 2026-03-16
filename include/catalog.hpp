#pragma once
#include "types.hpp"
#include <string>
#include <vector>

enum class SortField { Title, Year, Rating };

struct FilterOptions {
    std::string genre;
    float       minRating = 0.0f;
    std::string keyword;
};

void               addEntry(Catalog& cat, Entry e);
bool               editEntry(Catalog& cat, int id, const Entry& updated);
bool               deleteEntry(Catalog& cat, int id);
bool               toggleStatus(Catalog& cat, int id);
std::vector<Entry> queryEntries(const Catalog& cat, SortField sort, const FilterOptions& opts);
void               printTable(const std::vector<Entry>& entries, const std::string& label);
void               printEntry(const Entry& e);
