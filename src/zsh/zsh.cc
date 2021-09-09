#include "zsh/zsh.h"

#include <numeric>
#include <cassert>
#include <regex>
#include <set>

#include "zsh/cpp20.h"

namespace zsh
{
    void zsh::add(const std::string& path, i64 now)
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

    namespace
    {
	    i64 get_rank(const entry& e, i64 now, sort_algorithm sort)
        {
            switch(sort)
            {
            case sort_algorithm::rank:
                return static_cast<i64>(e.rank);
            case sort_algorithm::recent:
                return e.time - now;
            case sort_algorithm::frecent:
                {
                    // relate frequency and time
                    const auto dx = now - e.time;
                    return static_cast<i64>(10000 * e.rank * (3.75/((0.0001 * dx + 1) + 0.25)));
                }
            default:
                assert(false && "unhandled case");
                return 0;
            }
        }

        struct MatchSort
	    {
	        bool operator()(const match& lhs, const match& rhs) const
	        {
	            return lhs.rank < rhs.rank;
	        }
	    };

    // should be std::syntax_option_type
        using regex_args = decltype(std::regex::icase);
        struct match_result
        {
            std::optional<match> best;
            std::vector<std::regex> search;
            std::multiset<match, MatchSort> matches;

            match_result(const std::vector<std::string>& searches, regex_args args)
            {
                for(const auto& s: searches)
                {
                    search.emplace_back(s, args);
                }
            }
        };

        void update(match_result* results, const std::string& path, i64 rank, bool list)
        {
            if(list)
            {
                results->matches.emplace(match{path, rank});
            }

            if(results->best.has_value() == false || rank > results->best->rank)
            {
                results->best = {path, rank};
            }
        }

        bool match(const match_result& result, const std::string& path)
        {
            for(const auto& r: result.search)
            {
                if(std::regex_search(path, r) == false)
                {
                    return false;
                }
            }

            return true;
        }

        bool update_and_match(match_result* results, const std::string& path, i64 rank, bool list)
        {
            if(match(*results, path))
            {
                update(results, path, rank, list);
                return true;
            }

            return false;
        };
    }

    std::vector<::zsh::match> zsh::get(const std::vector<std::string>& search, i64 now, sort_algorithm sort, bool list)
    {
        constexpr auto regex_engine = std::regex::ECMAScript;
        
        auto c_match = match_result{search, regex_engine};
        auto i_match = match_result{search, regex_engine | std::regex::icase};

        for(const auto& e: entries)
        {
            if( update_and_match(&c_match, e.first, get_rank(e.second, now, sort), list) == false)
            {
                update_and_match(&i_match, e.first, get_rank(e.second, now, sort), list);
            }
        }

        auto compose = [list](const match_result& m) -> std::vector<::zsh::match>
        {
            if(list)
            {
                return std::vector(m.matches.begin(), m.matches.end());
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

    std::optional<std::string> zsh::get_single(const std::vector<std::string>& search, i64 now, sort_algorithm sort)
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

    std::vector<::zsh::match> zsh::get_all(const std::vector<std::string>& search, i64 now, sort_algorithm sort)
    {
	    return get(search, now, sort, true);
    }

}
