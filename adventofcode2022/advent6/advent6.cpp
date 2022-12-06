#include "advent6.h"
#include "../advent/advent_utils.h"

#define ENABLE_DAY6DBG 1
#ifdef NDEBUG
#define DAY6DBG 0
#else
#define DAY6DBG ENABLE_DAY6DBG
#endif

#if DAY6DBG
	#include <iostream>
#endif

namespace
{
#if DAY6DBG
	std::ostream & log = std::cout;
#else
	struct {	template <typename T> auto& operator<<(const T&) const noexcept { return *this; } } log;
#endif
}

#include <string_view>
#include <string>
#include <algorithm>
#include <array>

#include "ring_buffer.h"
#include "int_range.h"
#include "has_duplicates.h"

namespace
{
	constexpr std::size_t window_size = 4;

	

	template <std::size_t WINDOW_SIZE>
	int solve_generic(std::string_view input)
	{
		utils::ring_buffer<char, WINDOW_SIZE> seen_chars;
		seen_chars.fill('\0');
		std::size_t last_seen_cooldown = WINDOW_SIZE-1;
		for (std::size_t i : utils::int_range{ input.size() })
		{
			const char c = input[i];
			seen_chars.rotate(1);
			seen_chars.back() = c;
			if (!utils::ranges::has_duplicates(seen_chars))
			{
				const int result = static_cast<int>(i) + 1;
				return result;
			}
		}
		AdventUnreachable();
		return 0;
	}

	int solve_p1(std::string_view input)
	{
		return solve_generic<window_size>(input);
	}

	int solve_p1(std::istream& input)
	{	
		std::string in_string;
		std::getline(input,in_string);
		return solve_p1(in_string);
	}
}

namespace
{
	int solve_p2(std::string_view input)
	{
		return 0;
	}

	int solve_p2(std::istream& input)
	{
		std::string in_string;
		std::getline(input, in_string);
		return solve_p2(in_string);
	}

	std::string_view get_testcase_input(std::size_t input)
	{
		std::array input_array{
			"mjqjpqmgbljsphdztnvjfqwrcgsmlb",
			"bvwbjplbgvbhsrlpgdmjqwftvncz",
			"nppdvjthqldpwncqszvftbrmjlhg",
			"nznrnfrfntjfmvfwmzdfjlvtqnbhcprsg",
			"zcfzfwzzqfrljwzlrfnpqdbhtmscgvjw"
		};

		AdventCheck(input < input_array.size());
		return input_array[input];
	}
}

ResultType day_six_internal::day_six_p1_testcase(std::size_t input_idx)
{
	const std::string_view input = get_testcase_input(input_idx);
	return solve_p1(input);
}

ResultType advent_six_p1()
{
	auto input = advent::open_puzzle_input(6);
	return solve_p1(input);
}

ResultType advent_six_p2()
{
	auto input = advent::open_puzzle_input(6);
	return solve_p2(input);
}

#undef DAY6DBG
#undef ENABLE_DAY6DBG