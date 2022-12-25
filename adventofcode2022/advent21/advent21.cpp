#include "advent21.h"
#include "../advent/advent_utils.h"

#define ENABLE_DAY21DBG 1
#ifdef NDEBUG
#define DAY21DBG 0
#else
#define DAY21DBG ENABLE_DAY21DBG
#endif

#if DAY21DBG
	#include <iostream>
#endif

namespace
{
#if DAY21DBG
	std::ostream & log = std::cout;
#else
	struct {	template <typename T> auto& operator<<(const T&) const noexcept { return *this; } } log;
#endif
}

#include <optional>
#include <variant>
#include "sorted_vector.h"
#include "parse_utils.h"
#include "trim_string.h"
#include "to_value.h"
#include "istream_line_iterator.h"
#include "swap_remove.h"

namespace
{
	using Value = int64_t;
	constexpr std::string_view ROOT_ID = "root";
	constexpr std::string_view HUMAN_ID = "humn";

	struct Monkey
	{
	private:
#if DAY21DBG
		std::string id;
#else
		uint32_t value = 0;
#endif
	public:
		explicit Monkey(std::string_view str)
		{
			from_string(str);
		}
		Monkey() : Monkey{ "" } {}
		void from_string(std::string_view str)
		{
			str = utils::trim_string(str);
			AdventCheck(str.size() <= 4);
#if DAY21DBG
			id = std::string(str);
#else
			value = std::accumulate(begin(str), end(str), uint32_t{ 0 },
				[](uint32_t val, char c)
				{
					AdventCheck(c >= 0);
			const uint32_t rotated_val = val << CHAR_BIT;
			const uint32_t result = rotated_val + static_cast<uint32_t>(c);
			return result;
				});
#endif
		}

		std::string name() const
		{
#if DAY21DBG
			return id;
#else
			std::string result;
			result.reserve(4);
			uint32_t temp_val = value;
			for (int i = 0; i < 4; ++i)
			{
				constexpr uint32_t char_bitmask = 0xFF;
				const uint32_t val = temp_val & char_bitmask;
				if (val == 0) break;
				const char c_val = static_cast<char>(val);
				result.push_back(c_val);
				temp_val = temp_val >> CHAR_BIT;
			}
			std::ranges::reverse(result);
			return result;
#endif
		}
		auto operator<=>(const Monkey&) const = default;
	};


	struct MonkeyValue
	{
		Monkey monkey;
		Value value = 0;
		auto operator<=>(const MonkeyValue& other) const
		{
			return monkey <=> other.monkey;
		};
	};

	using ValueList = utils::sorted_vector<MonkeyValue, std::less<MonkeyValue>, 2>;

	const Value* get_value(const ValueList& list, Monkey monkey)
	{
		const auto result_it = list.find(MonkeyValue{ monkey,0 });
		return result_it != end(list) ? &(result_it->value) : nullptr;
	}

	enum class Operation : char
	{
		add = '+',
		sub = '-',
		mul = '*',
		div = '/',
		eql = '='
	};

	Operation to_operation(char c)
	{
		AdventCheck(std::string_view{ "+-*/" }.contains(c));
		const Operation result = static_cast<Operation>(c);
		return result;
	}

	Operation to_operation(std::string_view sv)
	{
		AdventCheck(sv.size() == 1u);
		const char c = sv[0];
		const Operation result = to_operation(c);
		return result;
	}

	struct DependantExpression
	{
		Monkey lhs;
		Monkey op_left, op_right;
		Operation op;

		std::optional<MonkeyValue> get_as_value(const ValueList& values) const
		{
			if (const auto left_ptr = get_value(values, op_left))
			{
				if (const auto right_ptr = get_value(values, op_right))
				{
					const Value left = *left_ptr;
					const Value right = *right_ptr;
					MonkeyValue result;
					result.monkey = lhs;
					switch (op)
					{
					default:
						AdventUnreachable();
						break;
					case Operation::add:
						result.value = left + right;
						break;
					case Operation::sub:
						result.value = left - right;
						break;
					case Operation::mul:
						result.value = left * right;
						break;
					case Operation::div:
						result.value = left / right;
						break;
					case Operation::eql:
						AdventCheck(op_left == op_right);
						AdventCheck(left == right);
						result.value = left;
						break;
					}
					return result;
				}
			}
			return std::nullopt;
		}

		constexpr auto operator<=>(const DependantExpression& other) const
		{
			return lhs <=> other.lhs;
		}
	};

	DependantExpression make_expression(Monkey lhs, Monkey op_left, Operation op, Monkey op_right)
	{
		DependantExpression result;
		result.lhs = std::move(lhs);
		result.op_left = std::move(op_left);
		result.op_right = std::move(op_right);
		result.op = op;
		return result;
	}

	DependantExpression to_dependant_expression(Monkey lhs, std::string_view line)
	{
		auto [l_op, op_str, r_op] = utils::get_string_elements(line, 0, 1, 2);
		Monkey op_left{ l_op };
		Monkey op_right{ r_op };
		const Operation op = to_operation(op_str);

		const DependantExpression result = make_expression(std::move(lhs), std::move(op_left), op, std::move(op_right));
		return result;
	}

	using ExpressionList = utils::small_vector<DependantExpression, 2>;

	ExpressionList get_all_expressions(const DependantExpression& de)
	{
		ExpressionList result;
		switch (de.op)
		{
		case Operation::add:
			result.push_back(make_expression(de.op_left, de.lhs, Operation::sub, de.op_right));
			result.push_back(make_expression(de.op_right, de.lhs, Operation::sub, de.op_left));
			break;
		case Operation::sub:
			result.push_back(make_expression(de.op_left, de.lhs, Operation::add, de.op_right));
			result.push_back(make_expression(de.op_right, de.op_left, Operation::sub, de.lhs));
			break;
		case Operation::mul:
			result.push_back(make_expression(de.op_left, de.lhs, Operation::div, de.op_right));
			result.push_back(make_expression(de.op_right, de.lhs, Operation::div, de.op_left));
			break;
		case Operation::div:
			result.push_back(make_expression(de.op_left, de.lhs, Operation::mul, de.op_right));
			result.push_back(make_expression(de.op_right, de.op_left, Operation::div, de.lhs));
			break;
		default:
			AdventUnreachable();
			break;
		}
		return result;
	}

	using DependancyList = std::vector<DependantExpression>;

	struct SolutionState
	{
		ValueList values;
		DependancyList dependencies;
		const Value* get_value(Monkey m) const { return ::get_value(values, m); }
		void add_value(MonkeyValue mv)
		{
			if (const auto prev_value_ptr = get_value(mv.monkey))
			{
				const Value prev_value = *prev_value_ptr;
				AdventCheck(prev_value == mv.value);
				return;
			}
			values.push_back(std::move(mv));
		}
	};

	template <AdventDay day>
	struct SolutionAdder
	{
	private:
		SolutionState& state;
	public:
		explicit SolutionAdder(SolutionState& solution) : state{ solution } {}
		void operator()(MonkeyValue monkey)
		{
			if constexpr (day == AdventDay::One)
			{
				state.add_value(std::move(monkey));
			}
			if constexpr (day == AdventDay::Two)
			{
				if (monkey.monkey != Monkey{ HUMAN_ID })
				{
					state.add_value(std::move(monkey));
				}
			}
		}
		void operator()(DependantExpression de)
		{
			if constexpr (day == AdventDay::One)
			{
				state.dependencies.push_back(std::move(de));
			}
			if constexpr (day == AdventDay::Two)
			{
				ExpressionList expression;
				if (de.lhs == Monkey{ ROOT_ID })
				{
					DependantExpression eql_expr;
					eql_expr.op = Operation::eql;

					eql_expr.lhs = de.op_left;
					eql_expr.op_left = eql_expr.op_right = de.op_right;
					state.dependencies.push_back(eql_expr);

					eql_expr.lhs = de.op_right;
					eql_expr.op_left = eql_expr.op_right = de.op_left;
					state.dependencies.push_back(eql_expr);
				}
				else
				{
					ExpressionList expressions = get_all_expressions(de);
					std::move(begin(expressions), end(expressions), std::back_inserter(state.dependencies));
					if (de.lhs != Monkey{ ROOT_ID })
					{
						state.dependencies.push_back(std::move(de));
					}
				}
			}
		}
	};

	Value solve_for_value(SolutionState state, Monkey target)
	{
		while (true)
		{
			if (const auto result_ptr = state.get_value(target))
			{
				const Value result = *result_ptr;
				return result;
			}

			bool resolution_happened = false;
			for (auto it = begin(state.dependencies); it != end(state.dependencies); ++it)
			{
				const DependantExpression& expr = *it;
				if (const auto left_ptr = get_value(state.values, expr.op_left))
				{
					if (const auto right_ptr = get_value(state.values, expr.op_right))
					{
						const MonkeyValue left_val{ expr.op_left, *left_ptr };
						const MonkeyValue right_val{ expr.op_right, *right_ptr };
						const ValueList small_vals{ left_val,right_val };
						const std::optional<MonkeyValue> resolve_result = expr.get_as_value(small_vals);
						AdventCheck(resolve_result.has_value());
						const MonkeyValue& new_value = resolve_result.value();
						state.add_value(new_value);
						utils::swap_remove(state.dependencies, it);
						resolution_happened = true;
						break;
					}
				}
			}
			AdventCheck(resolution_happened);
			if (!resolution_happened)
			{
				break;
			}
		}
		AdventUnreachable();
		return 0;
	}

	using LineParseResult = std::variant<MonkeyValue, DependantExpression>;

	LineParseResult parse_line(std::string_view line)
	{
		auto [lhs_str, rhs_str] = utils::split_string_at_first(line, ':');
		rhs_str = utils::trim_left(rhs_str);
		const Monkey lhs{ lhs_str };
		if (utils::is_value(rhs_str))
		{
			const Value val = utils::to_value<Value>(rhs_str);
			return MonkeyValue{ lhs,val };
		}
		else
		{
			const DependantExpression result = to_dependant_expression(lhs, rhs_str);
			return result;
		}
		AdventUnreachable();
	}

	template <AdventDay day>
	SolutionState read_file(std::istream& input)
	{
		SolutionState result;
		for (std::string_view line : utils::istream_line_range{ input })
		{
			LineParseResult parse_result = parse_line(line);
			std::visit(SolutionAdder<day>{ result }, std::move(parse_result));
		}
		return result;
	}

	int64_t solve_p1(std::istream& input)
	{
		const Monkey root_monkey{ ROOT_ID };
		SolutionState solution_state = read_file<AdventDay::One>(input);
		const Value result = solve_for_value(std::move(solution_state), root_monkey);
		return result;
	}
}

namespace
{
	int64_t solve_p2(std::istream& input)
	{
		SolutionState initial_state = read_file<AdventDay::Two>(input);
		const Monkey human{ HUMAN_ID };
		const Value result = solve_for_value(std::move(initial_state), human);
		return result;
	}
}

namespace
{
	std::istringstream testcase_a()
	{
		return std::istringstream{
			"root: pppw + sjmn\n"
			"dbpl: 5\n"
			"cczh : sllz + lgvd\n"
			"zczc : 2\n"
			"ptdq : humn - dvpt\n"
			"dvpt : 3\n"
			"lfqf : 4\n"
			"humn : 5\n"
			"ljgn : 2\n"
			"sjmn : drzm * dbpl\n"
			"sllz : 4\n"
			"pppw : cczh / lfqf\n"
			"lgvd : ljgn * ptdq\n"
			"drzm : hmdt - zczc\n"
			"hmdt : 32"
		};
	}
}

ResultType day_twentyone_p1_a()
{
	auto input = testcase_a();
	return solve_p1(input);
}

ResultType day_twentyone_p2_a()
{
	auto input = testcase_a();
	return solve_p2(input);
}

ResultType advent_twentyone_p1()
{
	auto input = advent::open_puzzle_input(21);
	return solve_p1(input);
}

ResultType advent_twentyone_p2()
{
	auto input = advent::open_puzzle_input(21);
	return solve_p2(input);
}

#undef DAY21DBG
#undef ENABLE_DAY21DBG