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

        template<typename TBest>
        struct match_result
        {
            std::vector<std::regex> regexes;

            match_result(const std::vector<std::string>& searches, regex_args args)
            {
                for(const auto& s: searches)
                {
                    regexes.emplace_back(s, args);
                }
            }

            void update(const std::string& path, i64 rank)
	        {
                static_cast<TBest*>(this)->update(path, rank);
	        }

	        bool search(const std::string& path)
	        {
	            for(const auto& r: regexes)
	            {
	                if(std::regex_search(path, r) == false)
	                {
	                    return false;
	                }
	            }

	            return true;
	        }

	        bool update_and_match(const std::string& path, i64 rank)
	        {
	            if(search(path))
	            {
	                update(path, rank);
	                return true;
	            }

	            return false;
	        }
        };

        struct match_result_single : match_result<match_result_single>
        {
            std::optional<match> best;

            match_result_single(const std::vector<std::string>& searches, regex_args args)
	            : match_result(searches, args)
            {
            }

            void update(const std::string& path, i64 rank)
	        {
	            if(best.has_value() == false || rank > best->rank)
	            {
	                best = {path, rank};
	            }
	        }

            [[nodiscard]] bool has_value() const
            {
	            return best.has_value();
            }
        };

        struct match_result_all : match_result<match_result_all>
        {
            std::multiset<match, match_sort> matches;

            match_result_all(const std::vector<std::string>& searches, regex_args args)
	            : match_result(searches, args)
            {
            }

            void update(const std::string& path, i64 rank)
	        {
	            matches.emplace(match{path, rank});
	        }

            [[nodiscard]] bool has_value() const
            {
                return matches.empty() == false;
            }
        };

        template<typename TBest>
        std::optional<TBest> find_best_match(const zsh& z, const std::vector<std::string>& search, i64 now, sort_algorithm sort)
        {
            constexpr regex_args regex_engine = std::regex::ECMAScript;

            auto case_logic        = TBest{search, regex_engine};
			auto ignore_case_logic = TBest{search, regex_engine | std::regex::icase};
            
            for(const auto& e: z.entries)
            {
	            if( case_logic.update_and_match(e.first, get_rank(e.second, now, sort)) == false)
	            {
	                case_logic.update_and_match(e.first, get_rank(e.second, now, sort));
	            }
            }

            if(case_logic.has_value()) { return std::move(case_logic); }
            if(ignore_case_logic.has_value()) { return std::move(ignore_case_logic); }
            return {};
        }
    }

    std::optional<std::string> zsh::get_single(const std::vector<std::string>& search, i64 now, sort_algorithm sort) const
    {
	    if(const auto r = find_best_match<match_result_single>(*this, search, now, sort); r.has_value())
        {
            return r->best->path;
        }

	    return {};
    }

    std::vector<match> zsh::get_all(const std::vector<std::string>& search, i64 now, sort_algorithm sort) const
    {
	    if(const auto r = find_best_match<match_result_all>(*this, search, now, sort); r.has_value())
        {
            return std::vector(r->matches.begin(), r->matches.end());
        }

	    return {};
    }

}
