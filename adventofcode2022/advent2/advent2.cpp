#include "advent2.h"
#include "../advent/advent_utils.h"

#define ENABLE_DAY2DBG 1
#ifdef NDEBUG
#define DAY2DBG 0
#else
#define DAY2DBG ENABLE_DAY2DBG
#endif

#if DAY2DBG
	#include <iostream>
#endif

namespace
{
#if DAY2DBG
	std::ostream & log = std::cout;
#else
	struct {	template <typename T> auto& operator<<(const T&) const noexcept { return *this; } } log;
#endif
}

#include "sorted_vector.h"
#include "enum_order.h"
#include "istream_line_iterator.h"
#include "range_contains.h"

#include <numeric>
#include <string_view>
#include <array>

namespace
{
	// Score for winning with this move.
	enum class Move : uint8_t
	{
		Rock		= 1,
		Paper		= 2,
		Scissors	= 3
	};

	Move to_move(char c)
	{
		const utils::flat_map<char, Move, std::less<char>, 6> move_lookup = []()
		{
			utils::flat_map<char, Move, std::less<char>, 6> move_lookup;
			move_lookup.insert_unique('A', Move::Rock);
			move_lookup.insert_unique('B', Move::Paper);
			move_lookup.insert_unique('C', Move::Scissors);
			move_lookup.insert_unique('X', Move::Rock); 
			move_lookup.insert_unique('Y', Move::Paper); 
			move_lookup.insert_unique('Z', Move::Scissors);
			return move_lookup;
		}();

		return move_lookup.at(c);
	}

	int get_move_score(Move move)
	{
		return static_cast<int>(move);
	}

	// Score for right winning.
	enum class MatchResult : uint8_t
	{
		LeftWins	= 0,
		Draw		= 3,
		RightWins	= 6
	};

	MatchResult to_intended_result(char c)
	{
		const utils::flat_map<char, MatchResult, std::less<char>, 3> lookup = []()
		{
			utils::flat_map<char, MatchResult, std::less<char>, 3> result;
			result.insert_unique('X', MatchResult::LeftWins);
			result.insert_unique('Y', MatchResult::Draw);
			result.insert_unique('Z', MatchResult::RightWins);
			return result;
		}();
		return lookup.at(c);
	}

	int get_result_score(MatchResult result)
	{
		return static_cast<int>(result);
	}

	struct ResultLookup
	{
	private:
		utils::flat_map<Move,MatchResult,utils::EnumSorter<Move>,3> results;
	public:
		ResultLookup(MatchResult vs_rock, MatchResult vs_paper, MatchResult vs_scissors)
		{
			results.insert_unique(Move::Rock,vs_rock);
			results.insert_unique(Move::Paper,vs_paper);
			results.insert_unique(Move::Scissors,vs_scissors);
		}
		MatchResult get_result(Move move) const
		{
			return results.at(move);
		}
	};

	using MatchLookup = utils::flat_map<Move, ResultLookup, utils::EnumSorter<Move>, 3>;
	MatchLookup get_match_lookup()
	{
		MatchLookup match_lookup;
		match_lookup.insert_unique(Move::Rock,		ResultLookup{ MatchResult::Draw,		MatchResult::RightWins,	MatchResult::LeftWins });
		match_lookup.insert_unique(Move::Paper,		ResultLookup{ MatchResult::LeftWins,	MatchResult::Draw,		MatchResult::RightWins });
		match_lookup.insert_unique(Move::Scissors,	ResultLookup{ MatchResult::RightWins,	MatchResult::LeftWins,	MatchResult::Draw });
		return match_lookup;
	}

	MatchResult play(Move left, Move right)
	{
		const MatchLookup match_lookup = get_match_lookup();
		return match_lookup.at(left).get_result(right);
	}

	int get_rights_score(Move left, Move right)
	{
		const MatchResult match_result = play(left,right);
		return get_move_score(right) + get_result_score(match_result);
	}

	int get_score_from_chars_p1(char left, char right)
	{
		AdventCheck(utils::range_contains_inc(left,'A','C'));
		AdventCheck(utils::range_contains_inc(right,'X','Z'));
		const Move left_move = to_move(left);
		const Move right_move = to_move(right);
		return get_rights_score(left_move,right_move);
	}

	int get_score_from_line_p1(std::string_view line)
	{
		AdventCheck(line.size() == std::size_t{3});
		AdventCheck(line[1] == ' ');
		return get_score_from_chars_p1(line[0],line[2]);
	}

	int score_match_p1(std::istream & input)
	{
		using ILI = utils::istream_line_iterator;
		const int result = std::transform_reduce(ILI{input},ILI{},0,std::plus<int>{},get_score_from_line_p1);
		return result;
	}

	int solve_p1(std::istream& input)
	{
		return score_match_p1(input);
	}
}

namespace
{
	int get_score_from_chars_p2(char left, char right)
	{
		AdventCheck(utils::range_contains_inc(left, 'A', 'C'));
		AdventCheck(utils::range_contains_inc(right, 'X', 'Z'));
		const Move left_move = to_move(left);
		const MatchResult intended_result = to_intended_result(right);
		const MatchLookup match_lookup = get_match_lookup();
		const Move right_move = [&result_lookup = match_lookup.at(left_move), intended_result]()
		{
			constexpr std::array<Move, 3> all_moves{ Move::Rock,Move::Paper,Move::Scissors };
			const auto move_it = std::find_if(begin(all_moves), end(all_moves), [&result_lookup, intended_result](Move m)
				{
					const MatchResult result = result_lookup.get_result(m);
					return result == intended_result;
				});
			AdventCheck(move_it != end(all_moves));
			return *move_it;
		}();
		return get_rights_score(left_move, right_move);
	}

	int get_score_from_line_p2(std::string_view line)
	{
		AdventCheck(line.size() == std::size_t{ 3 });
		AdventCheck(line[1] == ' ');
		return get_score_from_chars_p2(line[0], line[2]);
	}

	int score_match_p2(std::istream& input)
	{
		using ILI = utils::istream_line_iterator;
		const int result = std::transform_reduce(ILI{ input }, ILI{}, 0, std::plus<int>{}, get_score_from_line_p2);
		return result;
	}

	int solve_p2(std::istream& input)
	{
		return score_match_p2(input);
	}
}

namespace
{
	std::istringstream testcase_a()
	{
		return std::istringstream{ "A Y\nB X\nC Z" };
	}
}

ResultType day_two_p1_a()
{
	std::istringstream input = testcase_a();
	return solve_p1(input);
}

ResultType day_two_p2_a()
{
	std::istringstream input = testcase_a();
	return solve_p2(input);
}

ResultType advent_two_p1()
{
	auto input = advent::open_puzzle_input(2);
	return solve_p1(input);
}

ResultType advent_two_p2()
{
	auto input = advent::open_puzzle_input(2);
	return solve_p2(input);
}

#undef DAY2DBG
#undef ENABLE_DAY2DBG