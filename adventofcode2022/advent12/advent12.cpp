#include "advent12.h"
#include "../advent/advent_utils.h"

#define ENABLE_DAY12DBG 1
#ifdef NDEBUG
#define DAY12DBG 0
#else
#define DAY12DBG ENABLE_DAY12DBG
#endif

#if DAY12DBG
	#include <iostream>
#endif

namespace
{
#if DAY12DBG
	std::ostream & log = std::cout;
#else
	struct {	template <typename T> auto& operator<<(const T&) const noexcept { return *this; } } log;
#endif
}

#include "grid.h"
#include "range_contains.h"
#include "comparisons.h"

namespace
{
	using Grid = utils::grid<char>;
	using utils::coords;

	constexpr char START_POINT = 'S';
	constexpr char END_POINT = 'E';

	Grid get_grid(std::istream& input)
	{
		return utils::grid_helpers::build(input, [](char in) {return in; });
	}

	int get_height(char in)
	{
		AdventCheck(utils::range_contains_inc(in, 'a', 'z') || in == 'S' || in == 'E');
		switch (in)
		{
		case START_POINT:
			return 'a';
		case END_POINT:
			return 'z';
		default:
			break;
		}
		return in;
	}

	template <AdventDay day>
	std::optional<float> search_cost(coords from_pos, char from_node, coords to_pos, char to_node)
	{
		// Day 2 goes backwards. So reverse the args and pass to day one.
		if constexpr (day == AdventDay::Two)
		{
			return search_cost<AdventDay::One>(to_pos, to_node, from_pos, from_node);
		}
		if constexpr (day == AdventDay::One)
		{
			const int distance = from_pos.manhatten_distance(to_pos);
			AdventCheck(distance <= 2);
			if (distance != 1) return std::nullopt;
			const int from_height = get_height(from_node);
			const int to_height = get_height(to_node);
			if ((to_height - from_height) > 1)
			{
				return std::nullopt;
			}
			return 1.0f;
		}
		AdventUnreachable();
		return std::nullopt;
	}

	coords get_point(const Grid& grid, char c)
	{
		const std::optional<coords> result = grid.get_coordinates(c);
		AdventCheck(result.has_value());
		return result.value();
	};

	float get_heuristic(coords location, char node, coords end_point, char target_height)
	{
		const int distance = location.manhatten_distance(end_point);
		const int height_difference = std::abs(get_height(target_height) - get_height(node));
		return static_cast<float>(distance + height_difference);
	}

	int solve_p1(std::istream& input)
	{
		const Grid grid = get_grid(input);
		const coords start_point = get_point(grid, START_POINT);
		const coords ending_point = get_point(grid, END_POINT);

		auto heuristic_fn = [ending_point](coords location, char node)
		{
			return get_heuristic(location, node, ending_point, END_POINT);
		};

		const auto path = grid.get_path(start_point, ending_point, search_cost<AdventDay::One>, heuristic_fn);
		
		return static_cast<int>(path.size()) - 1;
	}
}

namespace
{
	// Search backwards from E until we find an 'a'

	bool is_at_end(coords location, char node)
	{
		const int height = get_height(node);
		return height == 'a';
	}

	int solve_p2(std::istream& input)
	{
		const Grid grid = get_grid(input);

		const coords start_point = get_point(grid, END_POINT);

		auto all_end_points = grid.get_all_coordinates_by_predicate([](char node) {return get_height(node) == 'a'; });
		auto heuristic_fn = [](coords location, char node) {return 0.0f; };
		// This heuristic is signicantly slower than using no heuristic. LOL.
		/*auto heuristic_fn = [&all_end_points](coords location, char node)
		{
			auto partial_heuristic = [location, node](coords end_point)
			{
				return get_heuristic(location, node, end_point, 'a');
			};
			const coords target = utils::min_transform(begin(all_end_points), end(all_end_points), partial_heuristic);
			return partial_heuristic(target);
		};*/

		const auto path = grid.get_path(start_point, is_at_end, search_cost<AdventDay::Two>,heuristic_fn);
		return static_cast<int>(path.size()) - 1;
	}
}

namespace
{
	std::istringstream testcase_a()
	{
		return std::istringstream{
			"Sabqponm\n"
			"abcryxxl\n"
			"accszExk\n"
			"acctuvwj\n"
			"abdefghi"
		};
	}
}

ResultType day_twelve_p1_a()
{
	auto input = testcase_a();
	return solve_p1(input);
}

ResultType day_twelve_p2_a()
{
	auto input = testcase_a();
	return solve_p2(input);
}

ResultType advent_twelve_p1()
{
	auto input = advent::open_puzzle_input(12);
	return solve_p1(input);
}

ResultType advent_twelve_p2()
{
	auto input = advent::open_puzzle_input(12);
	return solve_p2(input);
}

#undef DAY12DBG
#undef ENABLE_DAY12DBG