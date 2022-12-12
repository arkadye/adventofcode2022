#include "advent11.h"
#include "../advent/advent_utils.h"

#define ENABLE_DAY11DBG 1
#ifdef NDEBUG
#define DAY11DBG 0
#else
#define DAY11DBG ENABLE_DAY11DBG
#endif

#if DAY11DBG
	#include <iostream>
#endif

namespace
{
#if DAY11DBG
	std::ostream & log = std::cout;
#else
	struct {	template <typename T> auto& operator<<(const T&) const noexcept { return *this; } } log;
#endif
}

#include "istream_block_iterator.h"
#include "string_line_iterator.h"
#include "to_value.h"
#include "small_vector.h"
#include "parse_utils.h"
#include "split_string.h"
#include "int_range.h"
#include "range_contains.h"

#include <algorithm>
#include <variant>

namespace
{
	constexpr int WORRY_DIV_P1 = 3;
	constexpr int WORRY_DIV_P2 = 1;
	using Item = int64_t;
	using MonkeyId = std::size_t;

	struct OpArg_old {};
	struct OpArg_imm { int64_t arg; };
	using OpArg = std::variant<OpArg_old, OpArg_imm>;

	OpArg parse_arg(std::string_view arg)
	{
		if (utils::is_value(arg))
		{
			OpArg_imm result;
			result.arg = utils::to_value<int>(arg);
			return result;
		}
		else if (arg == "old")
		{
			return OpArg_old{};
		}
		AdventUnreachable();
		return OpArg_old{};
	}

	struct ArgVisitor
	{
	private:
		Item item;
	public:
		explicit ArgVisitor(Item i) : item{ i } {}
		int64_t operator()(OpArg_old) const { return item; }
		int64_t operator()(OpArg_imm arg) const { return arg.arg; }
	};

	int64_t get_arg(Item i, OpArg arg)
	{
		const int64_t result = std::visit(ArgVisitor{i}, arg);
		return result;
	}

	enum class OpType
	{
		add = '+',
		mul = '*'
	};

	OpType parse_optype(std::string_view instr)
	{
		AdventCheck(instr.size() == 1);
		const char inchar = instr[0];
		constexpr std::string_view legal_values = "+*";
		AdventCheck(legal_values.contains(inchar));
		return static_cast<OpType>(inchar);
	}

	class Operation
	{
		friend Operation parse_operation(std::string_view input);
		OpArg left = OpArg_old{};
		OpArg right = OpArg_old{};
		OpType type = OpType::add;
	public:
		Item apply(Item in) const
		{
			const Item l = get_arg(in, left);
			const Item r = get_arg(in, right);
			switch (type)
			{
			case OpType::add:
				return l + r;
			case OpType::mul:
				return l * r;
			default:
				break;
			}
			AdventUnreachable();
			return -1;
		}
	};

	Operation parse_operation(std::string_view input)
	{
		auto [left_str, op_str, right_str] = utils::get_string_elements(input, 0, 1, 2);
		Operation result;
		result.left = parse_arg(left_str);
		result.right = parse_arg(right_str);
		result.type = parse_optype(op_str);
		return result;
	}

	struct ItemThrow
	{
		Item item = 0;
		MonkeyId target_id = 0;
	};

	template <Item WorryDivider>
	class Monkey
	{
	private:
#if DAY11DBG
		MonkeyId id = 0;
#endif
		utils::small_vector<Item, 10> items;
		Operation operation;
		Item test_modulus = 0;
		MonkeyId true_target = 0;
		MonkeyId false_target = 0;

		int num_items_inspected = 0;
	public:
		void validate_id(MonkeyId expected_id)
		{
#if DAY11DBG
			AdventCheck(id == expected_id);
#endif
		}

		Item get_test_modulus() const { return test_modulus; }

		utils::small_vector<ItemThrow, 10> inspect_all_items(Item worry_ceiling)
		{
			utils::small_vector<ItemThrow, 10> result;
			auto inspect_item = [this,worry_ceiling](Item i)
			{
				AdventCheck((worry_ceiling % test_modulus) == 0);
				const Item inspected = operation.apply(i);
				const Item got_bored = inspected / WorryDivider;
				const Item capped_worry = got_bored % worry_ceiling;
				const bool test_result = ((capped_worry % test_modulus) == 0);
				ItemThrow result;
				result.item = capped_worry;
				result.target_id = test_result ? true_target : false_target;
				return result;
			};

			std::transform(begin(items), end(items), std::back_inserter(result), inspect_item);
			num_items_inspected += static_cast<int>(items.size());
			items.clear();
			return result;
		}

		void give_item(Item item)
		{
			items.push_back(item);
		}

		int get_inspected_items() const { return num_items_inspected; }

		void parse_monkey(std::string_view input)
		{
			using utils::split_string_at_first;
			using utils::remove_specific_prefix;
			using utils::remove_specific_suffix;
			using utils::to_value;

			auto get_next_line = [&input]()
			{
				const auto [next_line, body] = split_string_at_first(input, '\n');
				input = body;
				return next_line;
			};

			// Set ID.
			{
				std::string_view header_str = get_next_line();
#if DAY11DBG
				header_str = remove_specific_prefix(header_str, "Monkey ");
				header_str = remove_specific_suffix(header_str, ":");
				id = to_value<MonkeyId>(header_str);
#endif
			}

			// Set starting items
			{
				using SLI = utils::string_line_iterator;
				std::string_view items_str = get_next_line();
				items_str = remove_specific_prefix(items_str, "  Starting items: ");
				auto to_item = [](std::string_view in)
				{
					in = utils::trim_string(in);
					return to_value<Item>(in);
				};
				std::transform(SLI{ items_str, ',' }, SLI{}, std::back_inserter(items), to_item);
			}

			// Set operation
			{
				std::string_view op_line = get_next_line();
				op_line = remove_specific_prefix(op_line, "  Operation: new = ");
				operation = parse_operation(op_line);
			}

			// Set test modulus
			{
				std::string_view test_line = get_next_line();
				test_line = remove_specific_prefix(test_line, "  Test: divisible by ");
				test_modulus = to_value<Item>(test_line);
			}

			// Set true target
			{
				std::string_view true_line = get_next_line();
				true_line = remove_specific_prefix(true_line, "    If true: throw to monkey ");
				true_target = to_value<MonkeyId>(true_line);
			}

			// Set false target
			{
				std::string_view false_line = get_next_line();
				false_line = remove_specific_prefix(false_line, "    If false: throw to monkey ");
				false_target = to_value<MonkeyId>(false_line);
			}
		}
	};

	template <Item WorryDivider>
	using MonkeyContainer = utils::small_vector<Monkey<WorryDivider>, 7>;

	template <Item WorryDivider>
	MonkeyContainer<WorryDivider> parse_monkeys(std::istream& input)
	{
		using IBI = utils::istream_block_iterator;
		MonkeyContainer<WorryDivider> result;
		std::transform(IBI{ input }, IBI{}, std::back_inserter(result), [](std::string_view monkey_str)
			{
				Monkey<WorryDivider> result;
				result.parse_monkey(monkey_str);
				return result;
			});
		return result;
	}

	template <Item WorryDivider>
	Item calculate_worry_ceiling(const MonkeyContainer<WorryDivider>& monkeys)
	{
		const Item result = std::transform_reduce(begin(monkeys), end(monkeys), Item{ 1 },
			std::lcm<Item, Item>, [](const Monkey<WorryDivider>& monkey)
			{
				return monkey.get_test_modulus();
			});
		AdventCheck(result > 0);
		return result;
	}

	template <Item WorryDivider>
	void simulate_round(MonkeyContainer<WorryDivider>& monkeys)
	{
		const Item worry_ceiling = calculate_worry_ceiling(monkeys);
#if DAY11DBG
		for (std::size_t id : utils::int_range{ monkeys.size() })
		{
			Monkey<WorryDivider>& monkey = monkeys[id];
			monkey.validate_id(id);
#else
		for (Monkey<WorryDivider>& monkey : monkeys)
		{
#endif
			const auto throws = monkey.inspect_all_items(worry_ceiling);
			for (auto [item, target_id] : throws)
			{
				Monkey<WorryDivider>& target = monkeys[target_id];
				target.validate_id(target_id);
				target.give_item(item);
			}
		}
	}

	template <Item WorryDivider>
	void simlulate_n_rounds(MonkeyContainer<WorryDivider>& monkeys, int num_rounds)
	{
		AdventCheck(num_rounds >= 0);
		for (auto i : utils::int_range{ num_rounds })
		{
			simulate_round(monkeys);
		}
	}

	template <Item WorryDivider>
	int64_t calculate_monkey_business(MonkeyContainer<WorryDivider>& monkeys, std::size_t num_monkeys)
	{
		AdventCheck(utils::range_contains_inc(num_monkeys, std::size_t{ 0 }, monkeys.size()));

		auto monkey_order = [](const Monkey<WorryDivider>& left, const Monkey<WorryDivider>& right)
		{
			return left.get_inspected_items() > right.get_inspected_items();
		};
		auto mid_it = begin(monkeys) + num_monkeys;
		std::nth_element(begin(monkeys), mid_it, end(monkeys), monkey_order);
		const int64_t result = std::transform_reduce(
			begin(monkeys), mid_it,
			int64_t{ 1 },
			std::multiplies<int64_t>{},
			[](const Monkey<WorryDivider>& m) {return m.get_inspected_items(); });
		return result;
	}

	template <Item WorryDivisor>
	int64_t solve_generic(std::istream& input, int num_rounds)
	{
		auto monkeys = parse_monkeys<WorryDivisor>(input);
		simlulate_n_rounds(monkeys, num_rounds);
		const int64_t result = calculate_monkey_business(monkeys, 2);
		return result;
	}

	int64_t solve_p1(std::istream& input)
	{
		return solve_generic<WORRY_DIV_P1>(input, 20);
	}
}

namespace
{
	int64_t solve_p2(std::istream& input, int num_rounds)
	{
		return solve_generic<WORRY_DIV_P2>(input, num_rounds);
	}
}

ResultType day_eleven_p1_a()
{
	auto input = advent::open_testcase_input(11, 'a');
	return solve_p1(input);
}

ResultType day_11_internal::day_eleven_p2_a_generic(int num_rounds)
{
	auto input = advent::open_testcase_input(11, 'a');
	return solve_p2(input, num_rounds);
}

ResultType advent_eleven_p1()
{
	auto input = advent::open_puzzle_input(11);
	return solve_p1(input);
}

ResultType advent_eleven_p2()
{
	auto input = advent::open_puzzle_input(11);
	return solve_p2(input, 10000);
}

#undef DAY11DBG
#undef ENABLE_DAY11DBG