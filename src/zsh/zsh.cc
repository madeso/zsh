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

        struct match_sort
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
            std::multiset<match, match_sort> matches;

            match_result(const std::vector<std::string>& searches, regex_args args)
            {
                for(const auto& s: searches)
                {
                    search.emplace_back(s, args);
                }
            }

            void update(const std::string& path, i64 rank, bool list)
	        {
	            if(list)
	            {
	                matches.emplace(::zsh::match{path, rank});
	            }

	            if(best.has_value() == false || rank > best->rank)
	            {
	                best = {path, rank};
	            }
	        }

	        bool match(const std::string& path)
	        {
	            for(const auto& r: search)
	            {
	                if(std::regex_search(path, r) == false)
	                {
	                    return false;
	                }
	            }

	            return true;
	        }

	        bool update_and_match(const std::string& path, i64 rank, bool list)
	        {
	            if(match(path))
	            {
	                update(path, rank, list);
	                return true;
	            }

	            return false;
	        }
        };

        std::optional<match_result> find_best_match(const zsh& z, const std::vector<std::string>& search, i64 now, sort_algorithm sort, bool list)
        {
            constexpr regex_args regex_engine = std::regex::ECMAScript;

            auto case_logic        = match_result{search, regex_engine};
			auto ignore_case_logic = match_result{search, regex_engine | std::regex::icase};
            
            for(const auto& e: z.entries)
            {
	            if( case_logic.update_and_match(e.first, get_rank(e.second, now, sort), list) == false)
	            {
	                case_logic.update_and_match(e.first, get_rank(e.second, now, sort), list);
	            }
            }

            if(case_logic.best) { return std::move(case_logic); }
            if(ignore_case_logic.best) { return std::move(ignore_case_logic); }
            return {};
        }
    }

    std::optional<std::string> zsh::get_single(const std::vector<std::string>& search, i64 now, sort_algorithm sort) const
    {
	    if(const auto r = find_best_match(*this, search, now, sort, false); r.has_value())
        {
            return r->best->path;
        }

	    return {};
    }

    std::vector<match> zsh::get_all(const std::vector<std::string>& search, i64 now, sort_algorithm sort) const
    {
	    if(const auto r = find_best_match(*this, search, now, sort, true); r.has_value())
        {
            return std::vector(r->matches.begin(), r->matches.end());
        }

	    return {};
    }

}
