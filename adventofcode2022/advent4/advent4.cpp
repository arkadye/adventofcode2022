#include "advent4.h"
#include "../advent/advent_utils.h"

#define ENABLE_DAY4DBG 0
#ifdef NDEBUG
#define DAY4DBG 0
#else
#define DAY4DBG ENABLE_DAY4DBG
#endif

#if DAY4DBG
	#include <iostream>
#endif

namespace
{
#if DAY4DBG
	std::ostream & log = std::cout;
#else
	struct {	template <typename T> auto& operator<<(const T&) const noexcept { return *this; } } log;
#endif
}

#include <sstream>
#include <string_view>
#include <algorithm>

#include "split_string.h"
#include "int_range.h"
#include "istream_line_iterator.h"
#include "to_value.h"
#include "range_contains.h"

namespace
{
	using RangeVal = int;
	using Range = utils::int_range<RangeVal>;

	Range get_cleaning_range(std::string_view in)
	{
		const auto [sstart, sfinish] = utils::split_string_at_first(in, '-');
		const RangeVal start = utils::to_value<RangeVal>(sstart);
		const RangeVal finish = utils::to_value<RangeVal>(sfinish);
		AdventCheck(start <= finish);
		const Range result{ start,finish+1 };
		return result;
	}

	std::pair<Range, Range> get_range_pair(std::string_view in)
	{
		const auto [sleft, sright] = utils::split_string_at_first(in, ',');
		const Range left = get_cleaning_range(sleft);
		const Range right = get_cleaning_range(sright);
		return std::pair{ left,right };
	}

	enum class OverlapType
	{
		full,
		partial
	};

	template <OverlapType overlap_type>
	bool ranges_overlap(const Range& a, const Range& b)
	{
		auto impl = [](const Range& inner, const Range& outer)
		{
			if constexpr (overlap_type == OverlapType::full)
			{
				const bool low_matches = inner.front() >= outer.front();
				const bool high_matches = inner.back() <= outer.back();
				return low_matches && high_matches;
			}
			if constexpr (overlap_type == OverlapType::partial)
			{
				const bool low_matches = utils::range_contains_inc(inner.front(), outer.front(), outer.back());
				const bool high_matches = utils::range_contains_inc(inner.back(), outer.front(), outer.back());
				return low_matches || high_matches;
			}
			AdventUnreachable();
			return false;
		};

		return impl(a, b) || impl(b, a);
	}

	template <OverlapType overlap_type>
	int solve_generic(std::istream& input)
	{
		using ILI = utils::istream_line_iterator;
		auto line_has_overlap = [](std::string_view line)
		{
			const auto [left, right] = get_range_pair(line);
			const bool result = ranges_overlap<overlap_type>(left, right);
			log << "\nLine: '" << line
				<< "' Overlaps: " << (result ? "true" : "false")
				<< " Type: " << (overlap_type == OverlapType::full ? "full" : "partial");
			return result;
		};

		int result = std::count_if(ILI{ input }, ILI{}, line_has_overlap);

		return result;
	}

	int solve_p1(std::istream& input)
	{
		const int result = solve_generic<OverlapType::full>(input);
		return result;
	}

	int solve_p2(std::istream& input)
	{
		const int result = solve_generic<OverlapType::partial>(input);
		return result;
	}
}

namespace
{
	std::istringstream testcase_a()
	{
		return std::istringstream{
			"2-4,6-8\n"
			"2-3,4-5\n"
			"5-7,7-9\n"
			"2-8,3-7\n"
			"6-6,4-6\n"
			"2-6,4-8"
		};
	}
}

ResultType day_four_p1_a()
{
	auto input = testcase_a();
	return solve_p1(input);
}

ResultType day_four_p2_a()
{
	auto input = testcase_a();
	return solve_p2(input);
}

ResultType advent_four_p1()
{
	auto input = advent::open_puzzle_input(4);
	return solve_p1(input);
}

ResultType advent_four_p2()
{
	auto input = advent::open_puzzle_input(4);
	return solve_p2(input);
}

#undef DAY4DBG
#undef ENABLE_DAY4DBG