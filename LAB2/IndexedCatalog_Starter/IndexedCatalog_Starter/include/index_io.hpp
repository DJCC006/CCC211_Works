#pragma once

#include "catalog.hpp"

#include <filesystem>
#include <span>
#include <string>
#include <vector>

namespace lab2 {

struct IndexLoadResult {
    bool success{};
    std::vector<PrimaryEntry> entries;
    std::string error;
};

IndexLoadResult load_primary_index(const std::filesystem::path& path);

bool save_primary_index(
    const std::filesystem::path& path,
    std::span<const PrimaryEntry> entries,
    std::string& error);

} // namespace lab2
