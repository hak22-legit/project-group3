#pragma once
#include "types.hpp"
#include <optional>
#include <string>

std::optional<Entry> fetchOMDb(const std::string& title, int year = 0);