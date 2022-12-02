#include "advent1.h"
#include "../advent/advent_utils.h"

#define ENABLE_DAY1DBG 1
#ifdef NDEBUG
#define DAY1DBG 0
#else
#define DAY1DBG ENABLE_DAY1DBG
#endif

#if DAY1DBG
	#include <iostream>
#endif

#include <sstream>
#include <algorithm>
#include <numeric>
#include <limits>
#include <array>

#include "string_line_iterator.h"
#include "to_value.h"
#include "comparisons.h"
#include "istream_block_iterator.h"

namespace
{
#if DAY1DBG
	std::ostream & log = std::cout;
#else
	struct {	template <typename T> auto& operator<<(const T&) const noexcept { return *this; } } log;
#endif
}

namespace
{
	std::istringstream testcase_a()
	{
		return std::istringstream{
			"1000\n"
			"2000\n"
			"3000\n"
			"\n"
			"4000\n"
			"\n"
			"5000\n"
			"6000\n"
			"\n"
			"7000\n"
			"8000\n"
			"9000\n"
			"\n"
			"10000"
		};
	}
	
	using PayloadType = int;
	using ElfPayload = std::string;
	namespace stdr = std::ranges;

	PayloadType get_elf_payload(const ElfPayload& payload)
	{
		using SLI = utils::string_line_iterator;
		const PayloadType result = std::transform_reduce(SLI{payload},SLI{}, PayloadType{},std::plus<PayloadType>{},utils::to_value<PayloadType>);
		return result;
	}

	PayloadType get_biggest_payload(std::istream& input)
	{
		using IBI = utils::istream_block_iterator;
		const PayloadType result = std::transform_reduce(IBI{input},IBI{},std::numeric_limits<PayloadType>::min(),utils::Larger<PayloadType>{},get_elf_payload);
		return result;
	}

	PayloadType solve_p1(std::istream& input)
	{
		return get_biggest_payload(input);
	}
}

namespace
{
	static constexpr std::size_t TOP_N = 3;
	
	struct TopPayloads
	{
		std::array<PayloadType,TOP_N> data;
		TopPayloads()
		{
			std::ranges::fill(data,std::numeric_limits<PayloadType>::min());
		}
		void add_new_value(PayloadType new_value)
		{
			if (new_value > data.front())
			{
				data.front() = new_value;
				std::ranges::sort(data);
			}
		}
	};

	TopPayloads merge_top_payloads(const TopPayloads& left, const TopPayloads& right)
	{
		TopPayloads result = left;
		for (PayloadType payload : right.data)
		{
			result.add_new_value(payload);
		}
		return result;
	}

	PayloadType solve_p2(std::istream& input)
	{
		using IBI = utils::istream_block_iterator;
		const TopPayloads top_payloads = std::transform_reduce(IBI{input},IBI{},
			TopPayloads{},
			merge_top_payloads,
			[](const ElfPayload& elf_payload)
		{
			TopPayloads result;
			result.data.back() = get_elf_payload(elf_payload);
			return result;
		});

		const PayloadType result = std::accumulate(begin(top_payloads.data),end(top_payloads.data),PayloadType{0});
		return result;
	}
}

ResultType day_one_p1_a()
{
	auto input = testcase_a();
	return solve_p1(input);
}

ResultType day_one_p2_a()
{
	auto input = testcase_a();
	return solve_p2(input);
}

ResultType advent_one_p1()
{
	auto input = advent::open_puzzle_input(1);
	return solve_p1(input);
}

ResultType advent_one_p2()
{
	auto input = advent::open_puzzle_input(1);
	return solve_p2(input);
}

#undef DAY1DBG
#undef ENABLE_DAY1DBG