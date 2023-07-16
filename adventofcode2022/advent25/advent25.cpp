#include "advent25.h"
#include "../advent/advent_utils.h"

#define ENABLE_DAY25DBG 1
#ifdef NDEBUG
#define DAY25DBG 0
#else
#define DAY25DBG ENABLE_DAY25DBG
#endif

#if DAY25DBG
	#include <iostream>
#endif

namespace
{
#if DAY25DBG
	std::ostream & log = std::cout;
#else
	struct {	template <typename T> auto& operator<<(const T&) const noexcept { return *this; } } log;
#endif
}

#include <istream_line_iterator.h>
#include <numeric>

namespace
{
	constexpr int SNAFU_BASE = 5;
	constexpr int SNAFU_OFFSET = -2;
	using Decimal = int64_t;

	bool is_valid_snafu_char(char c)
	{
		constexpr std::string_view valid_chars = "=-012";
		return valid_chars.contains(c);
	}

	int snafu_char_to_digit(char c)
	{
		AdventCheck(is_valid_snafu_char(c));
		constexpr std::string_view digit_order = "=-012";
		static_assert(std::ssize(digit_order) == SNAFU_BASE);
		const std::size_t find_result = digit_order.find(c);
		AdventCheck(find_result < digit_order.size());
		const int result = static_cast<int>(find_result) + SNAFU_OFFSET;
		return result;
	}

	void validate_snafu(std::string_view snafu)
	{
		AdventCheck(std::ranges::all_of(snafu, is_valid_snafu_char));
	}

	int64_t snafu_to_decimal(std::string_view snafu)
	{
		validate_snafu(snafu);
		Decimal result = 0;
		for (char c : snafu)
		{
			result *= SNAFU_BASE;
			result += snafu_char_to_digit(c);
		}
		return result;
	}

	std::string decimal_to_snafu(Decimal decimal)
	{
		constexpr std::string_view snafu_digits = "012=-";
		static_assert(snafu_digits.size() == SNAFU_BASE);
		std::string result;
		while (decimal > 0)
		{
			const int digit_val = decimal % SNAFU_BASE;
			const char snafu_char = snafu_digits[digit_val];
			result.push_back(snafu_char);
			decimal /= SNAFU_BASE;
			if (!std::isdigit(snafu_char))
			{
				++decimal;
			}
		}
		std::ranges::reverse(result);
		return result;
	}

	std::string solve_p1(std::istream& input)
	{
		using ILI = utils::istream_line_iterator;
		const Decimal sum = std::transform_reduce(ILI{ input }, ILI{}, Decimal{ 0 }, std::plus<Decimal>{}, snafu_to_decimal);
		return decimal_to_snafu(sum);
	}

	std::istringstream testcase_a()
	{
		return std::istringstream{
			"1=-0-2\n"
			"12111\n"
			"2=0=\n"
			"21\n"
			"2=01\n"
			"111\n"
			"20012\n"
			"112\n"
			"1=-1=\n"
			"1-12\n"
			"12\n"
			"1=\n"
			"122"
		};
	}
}

ResultType day25_internal::day_twentyfive_p1_std(uint64_t snafuized)
{
	std::string result = std::to_string(snafuized);
	
	auto transform_char = [](char c)
	{
		switch (c)
		{
		case '0':
		case '1':
		case '2':
			return c;
		case '9':
			return '-';
		case '8':
			return '=';
		default:
			break;
		}
		AdventUnreachable();
		return '\0';
	};

	std::transform(begin(result), end(result), begin(result), transform_char);

	return snafu_to_decimal(result);
}

ResultType day25_internal::day_twentyfive_p1_dts(int arg)
{
	return decimal_to_snafu(arg);
}

ResultType day_twentyfive_p1_a()
{
	auto input = testcase_a();
	return solve_p1(input);
}

ResultType advent_twentyfive_p1()
{
	auto input = advent::open_puzzle_input(25);
	return solve_p1(input);
}

ResultType advent_twentyfive_p2()
{
	auto input = advent::open_puzzle_input(25);
	return "MERRY CHRISTMAS!";
}

#undef DAY25DBG
#undef ENABLE_DAY25DBG