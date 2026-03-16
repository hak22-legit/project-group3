#pragma once
#include "types.hpp"
#include <vector>
#include <string>

std::vector<User> loadUsers();
void              saveUsers(const std::vector<User>& users);
Catalog           loadCatalog(const std::string& username);
void              saveCatalog(const Catalog& cat);
