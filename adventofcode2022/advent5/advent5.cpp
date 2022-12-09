#include "advent5.h"
#include "../advent/advent_utils.h"

#define ENABLE_DAY5DBG 0
#ifdef NDEBUG
#define DAY5DBG 0
#else
#define DAY5DBG ENABLE_DAY5DBG
#endif

#if DAY5DBG
	#include <iostream>
#endif

namespace
{
#if DAY5DBG
	std::ostream & log = std::cout;
#else
	struct {	template <typename T> auto& operator<<(const T&) const noexcept { return *this; } } log;
#endif
}

#include <vector>
#include <algorithm>

#include "range_contains.h"
#include "int_range.h"
#include "istream_line_iterator.h"
#include "parse_utils.h"
#include "to_value.h"
#include "comparisons.h"

namespace
{
	struct Crate
	{
		char code = ' ';
	};

	std::ostream& operator<<(std::ostream& os, Crate c)
	{
		os << '[' << c.code << ']';
		return os;
	}

	struct CrateStack
	{
	private:
		friend struct CrateWarehouse;
		std::vector<Crate> crates;
	public:
		bool empty() const { return crates.empty(); }
		std::size_t size() const { return crates.size(); }
		void add_crate(Crate c) { crates.push_back(c); }
		Crate remove_crate()
		{
			AdventCheck(!empty());
			Crate c = crates.back();
			crates.pop_back();
			return c;
		}
		Crate operator[](std::size_t i) const
		{
			AdventCheck(i < size());
			return crates[i];
		}
		Crate from_top(std::size_t i) const
		{
			const std::size_t idx = size() - i - 1;
			return (*this)[idx];
		}
	};

	struct CrateWarehouse
	{
	private:
		std::vector<CrateStack> stacks;
	public:
		auto begin() const { return stacks.begin(); }
		auto end() const { return stacks.end(); }
		std::size_t size() const { return stacks.size(); }
		void move_one_crate(int from, int to)
		{
			AdventCheck(from != to);
			AdventCheck(utils::range_contains_inc(from, 1, static_cast<int>(stacks.size())));
			AdventCheck(utils::range_contains_inc(to  , 1, static_cast<int>(stacks.size())));

			const std::size_t from_idx = static_cast<std::size_t>(from) - 1;
			const std::size_t to_idx = static_cast<std::size_t>(to) - 1;
			const Crate c = stacks[from_idx].remove_crate();
			stacks[to_idx].add_crate(c);
		}

		template <AdventDay Day>
		void move_n_crates(int from, int to, int num)
		{
			AdventCheck(from != to);
			AdventCheck(utils::range_contains_inc(from, 1, static_cast<int>(stacks.size())));
			AdventCheck(utils::range_contains_inc(to  , 1, static_cast<int>(stacks.size())));
			AdventCheck(stacks[static_cast<std::size_t>(from)-1].size() >= static_cast<std::size_t>(num));

			if constexpr (Day == AdventDay::One)
			{
				AdventCheck(num > 0);
				for (auto i : utils::int_range{num})
				{
					move_one_crate(from,to);
				}
			}

			if constexpr (Day == AdventDay::Two)
			{
				if (num == 0) return;
				AdventCheck(num > 0);
				const std::size_t from_idx = static_cast<std::size_t>(from) - 1;
				const Crate crate = stacks[from_idx].remove_crate();

				move_n_crates<Day>(from, to, num - 1);

				const std::size_t to_idx = static_cast<std::size_t>(to) - 1;
				stacks[to_idx].add_crate(crate);
			}
		}

		void create_from_istream(std::istream& is)
		{
			constexpr std::size_t STACK_WIDTH = 4;
			while (true)
			{
				std::string line;
				std::getline(is,line);
				const std::size_t num_stacks = (line.size() / STACK_WIDTH) + (line.size() % STACK_WIDTH ? 1 : 0);
				if (num_stacks > stacks.size())
				{
					stacks.resize(num_stacks);
				}
				if(line.empty()) break;
				for (std::size_t str_idx : utils::int_range<std::size_t>{0,line.size(),STACK_WIDTH })
				{
					const char first_char = line[str_idx];
					if(first_char != '[') continue;
					const std::size_t stack_idx = str_idx / STACK_WIDTH;
					const char stack_char = line[str_idx + 1];
					const Crate new_crate{stack_char};
					stacks[stack_idx].crates.push_back(new_crate);
				}
			}

			for (CrateStack& stack : stacks)
			{
				std::ranges::reverse(stack.crates);
			}
		}
	};

	std::ostream& operator<<(std::ostream& os, const CrateWarehouse& warehouse)
	{
		const std::size_t biggest_stack = utils::ranges::max_transform(warehouse, [](const CrateStack& stack) {return stack.size(); }).size();
		for (std::size_t row_from_top : utils::int_range{ biggest_stack })
		{
			const std::size_t row_from_bottom = biggest_stack - row_from_top - 1;
			for (const CrateStack& stack : warehouse)
			{
				const bool has_crate = row_from_bottom < stack.size();
				if (has_crate)
				{
					os << stack[row_from_bottom];
				}
				else
				{
					os << "   ";
				}
				os << ' ';
			}
			os << '\n';
		}

		for (std::size_t stack_idx : utils::int_range{ warehouse.size() })
		{
			const int stack_num = static_cast<int>(stack_idx) + 1;
			if (stack_num < 10) os << ' ';
			os << stack_num;
			if (stack_num < 100) os << ' ';
			os << ' ';
		}
		os << '\n';
		return os;
	}

	template <AdventDay Day>
	CrateWarehouse move_crates_around(CrateWarehouse initial_warehouse, std::istream& iss)
	{
		for (std::string line : utils::istream_line_range{ iss })
		{
			log << '\n' << "Initial warehouse state:\n" << initial_warehouse
				<< "Processing: '" << line << "'\n";
			// Line is in form "move X from Y to Z"
			const auto [num_str,from_str,to_str] = utils::get_string_elements(line,1,3,5);
			const int num = utils::to_value<int>(num_str);
			const int from = utils::to_value<int>(from_str);
			const int to = utils::to_value<int>(to_str);
			initial_warehouse.move_n_crates<Day>(from,to,num);
		}
		log << "\nFinal state:\n" << initial_warehouse << '\n';
		return initial_warehouse;
	}

	template <AdventDay Day>
	CrateWarehouse get_moved_warehouse(std::istream& input)
	{
		CrateWarehouse result;
		result.create_from_istream(input);
		result = move_crates_around<Day>(std::move(result),input);
		return result;
	}

	template <AdventDay Day>
	std::string solve_generic(std::istream& input)
	{
		const CrateWarehouse warehouse = get_moved_warehouse<Day>(input);
		std::string result;
		result.reserve(warehouse.size());

		auto get_top_char = [](const CrateStack& stack)
		{
			const Crate crate = stack.from_top(0);
			return crate.code;
		};

		std::ranges::transform(warehouse,std::back_inserter(result),get_top_char);
		return result;
	}
}

namespace
{
	std::string solve_p1(std::istream& input)
	{
		return solve_generic<AdventDay::One>(input);
	}
	
	std::string solve_p2(std::istream& input)
	{
		return solve_generic<AdventDay::Two>(input);
	}
}

namespace {
	std::istringstream testcase_a()
	{
		return std::istringstream{
			"    [D]    \n"
			"[N] [C]    \n"
			"[Z] [M] [P]\n"
			" 1   2   3 \n"
			"\n"
			"move 1 from 2 to 1\n"
			"move 3 from 1 to 3\n"
			"move 2 from 2 to 1\n"
			"move 1 from 1 to 2"
		};
	}
}

ResultType day_five_p1_a()
{
	auto input = testcase_a();
	return solve_p1(input);
}

ResultType day_five_p2_a()
{
	auto input = testcase_a();
	return solve_p2(input);
}

ResultType advent_five_p1()
{
	auto input = advent::open_puzzle_input(5);
	return solve_p1(input);
}

ResultType advent_five_p2()
{
	auto input = advent::open_puzzle_input(5);
	return solve_p2(input);
}

#undef DAY5DBG
#undef ENABLE_DAY5DBG