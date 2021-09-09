#include <string>
#include <map>
#include <optional>
#include <cstdint>
#include <vector>

namespace zsh
{

using i64 = std::int64_t;

struct Entry
{
    double rank;
    i64 time;
};


enum class SortAlgorithm { Rank, Recent, Frecent };


struct Match
{
    std::string path;
    i64 rank;
};


struct Zsh
{
    std::map<std::string, Entry> entries;
    std::optional<int> max_score; // if null, then always age

    void add(const std::string& path, i64 now);

    std::vector<Match> get(const std::vector<std::string>& search, i64 now, SortAlgorithm sort, bool list);

    std::optional<std::string> get_single(const std::vector<std::string>& search, i64 now, SortAlgorithm sort);
    std::vector<Match> get_all(const std::vector<std::string>& search, i64 now, SortAlgorithm sort);
};

}
