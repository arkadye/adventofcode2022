#include "advent3.h"
#include "../advent/advent_utils.h"

#define ENABLE_DAY3DBG 1
#ifdef NDEBUG
#define DAY3DBG 0
#else
#define DAY3DBG ENABLE_DAY3DBG
#endif

#if DAY3DBG
	#include <iostream>
#endif

#include <algorithm>
#include <array>
#include <string_view>
#include <string>
#include <numeric>

#include "istream_line_iterator.h"
#include "range_contains.h"
#include "small_vector.h"

namespace
{
#if DAY3DBG
	std::ostream & log = std::cout;
#else
	struct {	template <typename T> auto& operator<<(const T&) const noexcept { return *this; } } log;
#endif
}

namespace
{
	using Item = char;
	using Rucksack = std::string;
	using RucksackView = std::string_view;

	void validate_item(Item item)
	{
		using utils::range_contains_inc;
		const bool is_low = range_contains_inc(item, 'a', 'z');
		const bool is_high = range_contains_inc(item, 'A', 'Z');
		AdventCheck(is_low || is_high);
	}

	void validate_rucksack(RucksackView rucksack)
	{
		AdventCheck(!rucksack.empty());
		AdventCheck(std::ranges::is_sorted(rucksack));
		std::ranges::for_each(rucksack, validate_item);
	}

	int get_item_priority(Item item)
	{
		validate_item(item);

		using utils::range_contains_inc;
		if (range_contains_inc(item, 'a', 'z'))
		{
			const int idx = item - 'a';
			return idx + 1;
		}
		else if (range_contains_inc(item, 'A', 'Z'))
		{
			const int idx = item - 'A';
			return idx + 27;
		}
		AdventUnreachable();
		return 0;
	}

	template <typename Container>
	Rucksack get_items_in_all(const Container& rucksacks)
	{
		std::ranges::for_each(rucksacks, validate_rucksack);
		auto combine = [](RucksackView a, RucksackView b)
		{
			if (a.empty())
			{
				return Rucksack{ b };
			}
			if (b.empty())
			{
				return Rucksack{ a };
			}
			Rucksack result;
			result.reserve(std::max(a.size(), b.size()));
			std::ranges::set_intersection(a, b, std::back_inserter(result));
			return result;
		};

		Rucksack result = std::reduce(begin(rucksacks), end(rucksacks), Rucksack{}, combine);
		const auto new_back = std::unique(begin(result), end(result));
		result.erase(new_back, end(result));
		return result;
	}

	Item get_item_in_both(RucksackView a, RucksackView b)
	{
		const std::array<RucksackView, 2> rucksacks{ a,b };
		const Rucksack items_in_both = get_items_in_all(rucksacks);
		AdventCheck(items_in_both.size() == std::size_t{ 1 });
		return items_in_both.front();
	}

	std::pair<RucksackView, RucksackView> get_compartments(std::string& rucksack)
	{
		const std::size_t rucksack_size = rucksack.size();
		AdventCheck(rucksack_size % 2 == 0);

		const std::size_t compartment_size = rucksack_size / 2;
		const auto start_it = begin(rucksack);
		const auto split_it = start_it + compartment_size;
		const auto finish_it = end(rucksack);

		std::sort(start_it, split_it);
		std::sort(split_it, finish_it);

		RucksackView compartment_a(start_it, split_it);
		RucksackView compartment_b(split_it, finish_it);

		validate_rucksack(compartment_a);
		validate_rucksack(compartment_b);
		return std::pair{ compartment_a,compartment_b };
	}

	int get_rucksack_priority(std::string line)
	{
		auto [compartment_a, compartment_b] = get_compartments(line);
		const Item matching_item = get_item_in_both(compartment_a, compartment_b);
		const int priority = get_item_priority(matching_item);
		return priority;
	}

	int solve_p1(std::istream& input)
	{
		using ILI = utils::istream_line_iterator;
		const int result = std::transform_reduce(ILI{ input }, ILI{}, 0, std::plus<int>{}, get_rucksack_priority);
		return result;
	}
}

namespace
{
	constexpr std::size_t GROUP_SIZE = 3;
	using ElfGroup = std::array<Rucksack, GROUP_SIZE>;
	std::array<Rucksack, GROUP_SIZE> get_next_group(std::istream& input)
	{
		using ILI = utils::istream_line_iterator;
		ElfGroup result;
		std::copy_n(ILI{ input }, result.size(), begin(result));
		for (Rucksack& rucksack : result)
		{
			std::ranges::sort(rucksack);
			validate_rucksack(rucksack);
		}
		return result;
	}

	Item get_shared_item(const ElfGroup& group)
	{
		const Rucksack shared_items = get_items_in_all(group);
		AdventCheck(shared_items.size() == std::size_t{ 1 });
		return shared_items.front();
	}

	int solve_p2(std::istream& input)
	{
		int result = 0;
		while (!input.eof())
		{
			const ElfGroup group = get_next_group(input);
			const Item shared_item = get_shared_item(group);
			const int item_priority = get_item_priority(shared_item);
			result += item_priority;
		}
		return result;
	}
}

namespace
{
	std::istringstream testcase_a()
	{
		return std::istringstream{
			"vJrwpWtwJgWrhcsFMMfFFhFp\n"
			"jqHRNqRjqzjGDLGLrsFMfFZSrLrFZsSL\n"
			"PmmdzqPrVvPwwTWBwg\n"
			"wMqvLMZHhHMvwLHjbvcjnnSBnvTQFn\n"
			"ttgJtRGJQctTZtZT\n"
			"CrZsJsPPZsGzwwsLwLmpwMDw"
		};
	}
}

ResultType day_three_p1_a()
{
	auto input = testcase_a();
	return solve_p1(input);
}

ResultType day_three_p2_a()
{
	auto input = testcase_a();
	return solve_p2(input);
}

ResultType advent_three_p1()
{
	auto input = advent::open_puzzle_input(3);
	return solve_p1(input);
}

ResultType advent_three_p2()
{
	auto input = advent::open_puzzle_input(3);
	return solve_p2(input);
}

#undef DAY3DBG
#undef ENABLE_DAY3DBG