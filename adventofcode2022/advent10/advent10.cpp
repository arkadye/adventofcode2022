#include "advent10.h"
#include "../advent/advent_utils.h"

#define ENABLE_DAY10DBG 1
#ifdef NDEBUG
#define DAY10DBG 0
#else
#define DAY10DBG ENABLE_DAY10DBG
#endif

#if DAY10DBG
	#include <iostream>
#endif

namespace
{
#if DAY10DBG
	std::ostream & log = std::cout;
#else
	struct {	template <typename T> auto& operator<<(const T&) const noexcept { return *this; } } log;
#endif
}

#include "istream_line_iterator.h"
#include "sorted_vector.h"
#include "split_string.h"
#include "to_value.h"
#include "int_range.h"
#include "range_contains.h"

#include <algorithm>
#include <numeric>
#include <string_view>
#include <string>
#include <array>

namespace
{
	enum class OpCode
	{
		invalid, noop, addx
	};

	int get_execution_time(OpCode oc)
	{
		switch (oc)
		{
		case OpCode::noop:
			return 1;
		case OpCode::addx:
			return 2;
		default:
			break;
		}
		AdventUnreachable();
		return -1;
	}

	OpCode to_opcode(std::string_view in)
	{
		if (in == "noop") return OpCode::noop;
		if (in == "addx") return OpCode::addx;
		AdventUnreachable();
		return OpCode::invalid;
	}

	struct Instruction
	{
		OpCode opcode = OpCode::invalid;
		int arg = 0;
	};

	Instruction to_instruction(std::string_view in)
	{
		const auto [opstr, argstr] = utils::split_string_at_first(in, ' ');
		
		Instruction result;
		result.opcode = to_opcode(opstr);
		result.arg = utils::to_value<int>(argstr);
		return result;
	}

	class Computer
	{
		Instruction instruction;
		int cycle_counter = 1;
		int reg_x = 1;
		int remaining_instruction_time = 0;

		void execute()
		{
			switch (instruction.opcode)
			{
			case OpCode::noop:
				break;
			case OpCode::addx:
				reg_x += instruction.arg;
				break;
			default:
				AdventUnreachable();
				break;
			}
			instruction.opcode = OpCode::invalid;
		}

	public:
		void set_instruction(Instruction i)
		{
			instruction = i;
			remaining_instruction_time = get_execution_time(i.opcode);
		}
		bool needs_instruction() const { return instruction.opcode == OpCode::invalid; }
		int get_reg_x() const { return reg_x; }
		int get_cycle() const { return cycle_counter; }
		int get_signal_strength() const { return get_reg_x() * get_cycle(); }
		void run_cycle()
		{
			++cycle_counter;
			--remaining_instruction_time;
			if (remaining_instruction_time == 0)
			{
				execute();
			}
		}
	};

	// Return the value of x during the nth cycles in the given program. INT_MAX for end of program.
	template <typename OutIterator>
	int run_program_p1(std::istream& input, OutIterator out)
	{
		auto should_log_value = [](int cycle_num)
		{
			const int base = cycle_num - 20;
			if (base < 0) return false;
			return (base % 40) == 0;
		};

		Computer computer;
		for(auto line : utils::istream_line_range{input})
		{
			AdventCheck(computer.needs_instruction());
			const Instruction i = to_instruction(line);
			computer.set_instruction(i);

			while (!computer.needs_instruction())
			{
				if (should_log_value(computer.get_cycle()))
				{
					*out = computer.get_signal_strength();
				}
				computer.run_cycle();
			}
		}
		return computer.get_reg_x();
	}


	int solve_p1(std::istream& input)
	{
		utils::small_vector<int,6> result_array;
		run_program_p1(input, std::back_inserter(result_array));
		const int result = std::accumulate(begin(result_array), end(result_array), 0);
		return result;
	}
}

namespace
{
	std::string solve_p2(std::istream& input, char lit, char unlit)
	{
		constexpr int WIDTH = 40;
		constexpr int HEIGHT = 6;
		std::string result;
		result.reserve(HEIGHT * WIDTH + HEIGHT - 1);

		Computer computer;
		for (auto line : utils::istream_line_range{ input })
		{
			AdventCheck(computer.needs_instruction());
			const Instruction instruction = to_instruction(line);
			computer.set_instruction(instruction);

			while (!computer.needs_instruction())
			{
				const int cycle = computer.get_cycle();
				const int active_pixel = (cycle - 1) % WIDTH;
				const int sprite_start = computer.get_reg_x() - 1;
				const int sprite_end = computer.get_reg_x() + 1;
				const bool is_pixel_lit = utils::range_contains_inc(active_pixel, sprite_start, sprite_end);
				if (active_pixel == 0)
				{
					result.push_back('\n');
				}
				result.push_back(is_pixel_lit ? lit : unlit);

				computer.run_cycle();
			}
		}
		return result;
	}
}

namespace
{
	std::istringstream testcase_a()
	{
		return std::istringstream{
			"noop\naddx 3\naddx -5"
		};
	}

	std::ifstream testcase_b()
	{
		return advent::open_testcase_input(10, 'b');
	}
}

ResultType day_ten_p1_a()
{
	auto input = testcase_a();
	utils::small_vector<int, 1> scratch;
	return run_program_p1(input, std::back_inserter(scratch));
}

ResultType day_ten_p1_b()
{
	auto input = testcase_b();
	if (!input.is_open())
	{
		return "INPUT NOT FOUND";
	}
	return solve_p1(input);
}

ResultType day_ten_p2_b()
{
	auto input = testcase_b();
	if (!input.is_open())
	{
		return "INPUT NOT FOUND";
	}
	return solve_p2(input,'#','.');
}

ResultType advent_ten_p1()
{
	auto input = advent::open_puzzle_input(10);
	return solve_p1(input);
}

ResultType advent_ten_p2()
{
	auto input = advent::open_puzzle_input(10);
	return solve_p2(input,'#',' ');
}

#undef DAY10DBG
#undef ENABLE_DAY10DBG