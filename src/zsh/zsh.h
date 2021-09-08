#include <string>
#include <map>
#include <optional>
#include <cstdint>
#include <vector>

// z.sh "port"
// original https://github.com/rupa/z

namespace zsh
{

using i64 = std::int64_t;

struct ZshEntry
{
    double rank;
    int time;
};


enum class SortAlgorithm { Rank, Recent, Frecent };


struct Match
{
    std::string path;
    i64 rank;
};


struct Zsh
{
    std::map<std::string, ZshEntry> entries;
    std::optional<int> max_score; // if null, then always age

    void add(const std::string& path, int now);

    std::vector<Match> get(const std::vector<std::string>& search, int now, SortAlgorithm sort, bool list);
};

}
