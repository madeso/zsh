#pragma once

#include <string>
#include <map>
#include <optional>
#include <cstdint>
#include <vector>

namespace zsh
{

using i64 = std::int64_t;

struct entry
{
    double rank;
    i64 time;
};


enum class sort_algorithm { rank, recent, frecent };


struct match
{
    std::string path;
    i64 rank;
};


struct zsh
{
    std::map<std::string, entry> entries;
    std::optional<int> max_score; // if null, then always age

    void add(const std::string& path, i64 now);

    std::optional<std::string> get_single(const std::vector<std::string>& search, i64 now, sort_algorithm sort) const;
    std::vector<match> get_all(const std::vector<std::string>& search, i64 now, sort_algorithm sort) const;
};

}
