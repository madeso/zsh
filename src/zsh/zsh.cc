#include "zsh/zsh.h"

#include <numeric>
#include <cassert>
#include <regex>
#include <set>

#include "zsh/cpp20.h"

namespace zsh
{
    void Zsh::add(const std::string& path, i64 now)
    {
        // add path to entries, or update the existing entry
        {
            constexpr double initial = 1.0;
            constexpr double increase = 1.0;

            auto found = entries.find(path);
            if(found != entries.end())
            {
                found->second.rank += increase;
                found->second.time = now;
            }
            else
            {
                entries[path] = {initial + increase, now};
            }
        }

        // drop ranks below 1
        erase_if(entries, [](const auto& e)
        {
            return e.second.rank < 1.0;
        });
        
        // aging
        auto age_entries = [this]()
        {
            for(auto& entry: entries)
            {
                entry.second.rank = 0.99 * entry.second.rank;
            }
        };
        if(max_score.has_value() == false)
        {
            age_entries();
        }
        else
        {
            const auto total_score = std::accumulate(entries.begin(), entries.end(), 0.0,
                [](double d, const auto& a) -> double
                {
                    return d + a.second.rank;
                }
            );

            if( total_score > *max_score )
            {
                age_entries();
            }
        }
    }
    

    std::vector<Match> Zsh::get(const std::vector<std::string>& search, i64 now, SortAlgorithm sort, bool list)
    {
        auto get_rank = [sort, now](const ZshEntry& e) -> i64
        {
            switch(sort)
            {
            case SortAlgorithm::Rank:
                return static_cast<i64>(e.rank);
            case SortAlgorithm::Recent:
                return e.time - now;
            case SortAlgorithm::Frecent:
                {
                    // relate frequency and time
                    const auto dx = now - e.time;
                    return static_cast<i64>(10000 * e.rank * (3.75/((0.0001 * dx + 1) + 0.25)));
                }
            default:
                assert(false && "unhandled case");
                return 0;
            }
        };

        struct MatchSort
        {
            bool operator()(const Match& lhs, const Match& rhs) const
            {
                return lhs.rank < rhs.rank;
            }
        };

        // should be std::syntax_option_type
        using regex_args = decltype(std::regex::icase);
        struct MatchResult
        {
            std::optional<Match> best;
            std::vector<std::regex> search;
            std::multiset<Match, MatchSort> matches;

            MatchResult(const std::vector<std::string>& searches, regex_args args)
            {
                for(const auto& s: searches)
                {
                    search.emplace_back(s, args);
                }
            }
        };

        auto update = [list](MatchResult* results, const std::string& path, i64 rank)
        {
            if(list)
            {
                results->matches.emplace(Match{path, rank});
            }

            if(results->best.has_value() == false || rank > results->best->rank)
            {
                results->best = {path, rank};
            }
        };

        auto match = [](const MatchResult& result, const std::string& path)
        {
            for(const auto& r: result.search)
            {
                if(std::regex_search(path, r) == false)
                {
                    return false;
                }
            }

            return true;
        };

        auto update_and_match = [update, match](MatchResult* results, const std::string& path, i64 rank)
        {
            if(match(*results, path))
            {
                update(results, path, rank);
                return true;
            }

            return false;
        };

        constexpr auto regex_engine = std::regex::ECMAScript;
        
        auto c_match = MatchResult{search, regex_engine};
        auto i_match = MatchResult{search, regex_engine | std::regex::icase};

        for(const auto& e: entries)
        {
            if( update_and_match(&c_match, e.first, get_rank(e.second)) == false)
            {
                update_and_match(&i_match, e.first, get_rank(e.second));
            }
        }

        auto compose = [list](const MatchResult& m) -> std::vector<Match>
        {
            if(list)
            {
                return std::vector<Match>(m.matches.begin(), m.matches.end());
            }
            else
            {
                if(m.best) { return {*m.best}; }
                else return {};
            }
        };

        if(c_match.best) { return compose(c_match); }
        else if(i_match.best) { return compose(i_match); }
        else return {};
    }

    std::optional<std::string> Zsh::get_single(const std::vector<std::string>& search, i64 now, SortAlgorithm sort)
    {
	    auto r = get(search, now, sort, false);
        if(r.empty())
        {
	        return {};
        }
        else
        {
	        return r[0].path;
        }
    }

    std::vector<Match> Zsh::get_all(const std::vector<std::string>& search, i64 now, SortAlgorithm sort)
    {
	    return get(search, now, sort, true);
    }

}
