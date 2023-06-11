#include "advent19.h"
#include "../advent/advent_utils.h"

#define ENABLE_DAY19DBG 1
#ifdef NDEBUG
#define DAY19DBG 0
#else
#define DAY19DBG ENABLE_DAY19DBG
#endif

#if DAY19DBG or true
	#include <iostream>
#endif

namespace
{
#if DAY19DBG
	std::ostream & log = std::cout;
#else
	struct {	template <typename T> auto& operator<<(const T&) const noexcept { return *this; } } log;
#endif
}

#include "istream_line_iterator.h"
#include "parse_utils.h"
#include "trim_string.h"
#include "small_vector.h"
#include "string_line_iterator.h"
#include "to_value.h"
#include "int_range.h"
#include "swap_remove.h"
#include "comparisons.h"

#include <future>

namespace
{
	using Quality = int;
	using ID = int;

	enum class RockType :char
	{
		ore,
		clay,
		obsidian,
		geode,
		INVALID
	};

	constexpr RockType TARGET_ROCK = RockType::geode;

	void validate(RockType rt)
	{
		AdventCheck(rt != RockType::INVALID);
	}

	constexpr std::size_t to_idx_impl(RockType rt)
	{
		return static_cast<std::size_t>(rt);
	}

	std::size_t to_idx(RockType rt)
	{
		validate(rt);
		return to_idx_impl(rt);
	}

	constexpr std::size_t num_rock_types()
	{
		return to_idx_impl(RockType::INVALID);
	}

	constexpr std::array<RockType, num_rock_types()> ROCK_TYPE_ARRAY = {
		RockType::ore, RockType::clay, RockType::obsidian, RockType::geode
	};

	template <typename MappedType>
	class RockTypeMap
	{
		std::array<MappedType, num_rock_types()> data{};
	public:
		MappedType& operator[](RockType rt)
		{
			const std::size_t idx = to_idx(rt);
			return data[idx];
		}

		const MappedType& operator[](RockType rt) const
		{
			const std::size_t idx = to_idx(rt);
			return data[idx];
		}

		template <typename OtherT>
		RockTypeMap& operator-=(const RockTypeMap<OtherT>& other)
		{
			for (RockType r : ROCK_TYPE_ARRAY)
			{
				(*this)[r] -= other[r];
			}
			return *this;
		}

		constexpr std::size_t size() const { return num_rock_types(); }

		constexpr auto begin() const { return data.begin(); }
		constexpr auto end() const { return data.end(); }
		constexpr auto begin() { return data.begin(); }
		constexpr auto end() { return data.end(); }
	};

	template <typename T>
	constexpr auto begin(const RockTypeMap<T>& rtm) { return rtm.begin(); }

	template <typename T>
	constexpr auto end(const RockTypeMap<T>& rtm) { return rtm.end(); }

	template <typename T>
	constexpr auto begin(RockTypeMap<T>& rtm) { return rtm.begin(); }

	template <typename T>
	constexpr auto end(RockTypeMap<T>& rtm) { return rtm.end(); }

	template <typename MappedType>
	constexpr auto operator<=>(const RockTypeMap<MappedType>& left, const RockTypeMap<MappedType>& right)
	{
		return std::lexicographical_compare_three_way(begin(left), end(left), begin(right), end(right));
	}

	RockTypeMap<std::string_view> get_rock_names()
	{
		RockTypeMap<std::string_view> rocks;
		rocks[RockType::ore] = "ore";
		rocks[RockType::clay] = "clay";
		rocks[RockType::obsidian] = "obsidian";
		rocks[RockType::geode] = "geode";
		return rocks;
	}

	RockType to_rock_type(std::string_view sv)
	{
		const RockTypeMap<std::string_view> names = get_rock_names();
		for (RockType rt : ROCK_TYPE_ARRAY)
		{
			if (names[rt] == sv)
			{
				return rt;
			}
		}
		return RockType::INVALID;
	}

	std::string_view to_str(RockType rt)
	{
		const RockTypeMap<std::string_view> names = get_rock_names();
		return names[rt];
	}

	RockType to_rock_type(std::size_t idx)
	{
		AdventCheck(idx < num_rock_types());
		return static_cast<RockType>(idx);
	}

	using Recipe = RockTypeMap<int8_t>;

	void check_empty(Recipe r)
	{
		for (RockType rock : ROCK_TYPE_ARRAY)
		{
			AdventCheck(r[rock] == 0);
		}
	}

	std::pair<RockType, Recipe> get_recipe_info(std::string_view recipe_str)
	{
		std::pair<RockType, Recipe> result;
		using namespace utils;
		recipe_str = remove_specific_prefix(recipe_str, " Each ");
		auto [product_type_str, requirements] = split_string_at_first(recipe_str, ' ');
		result.first = to_rock_type(product_type_str);

		requirements = remove_specific_prefix(requirements, "robot costs ");

		for (std::string_view requirment_str : string_line_range( requirements," and " ))
		{
			const auto [num_str, type_str] = split_string_at_first(requirment_str, ' ');
			const RockType type = to_rock_type(type_str);
			const int8_t amount = to_value<int8_t>(num_str);

			AdventCheck(result.second[type] == 0);
			result.second[type] = amount;
		}
		return result;
	}

	struct MiningState
	{
		RockTypeMap<int> num_bots;
		RockTypeMap<int> resources;
		constexpr auto operator<=>(const MiningState&) const = default;
		bool dominates(const MiningState& other) const
		{
			for (RockType type : ROCK_TYPE_ARRAY)
			{
				if (num_bots[type] < other.num_bots[type]) return false;
				if (resources[type] < other.resources[type]) return false;
			}
			return true;
		}
	};

	class Blueprint
	{
		ID id = 0;
		RockTypeMap<Recipe> recipes;

		void validate() {}

	public:
		auto begin() const { return recipes.begin(); }
		auto end() const { return recipes.end(); }
		explicit Blueprint(std::string_view line)
		{
			using namespace utils;
			auto [id_str, recipes_str] = split_string_at_first(line, ':');
			id_str = remove_specific_prefix(id_str, "Blueprint ");
			id = utils::to_value<ID>(id_str);

			for (std::string_view recipe_str : utils::string_line_range{ recipes_str,'.' })
			{
				const auto [builder_type, recipe] = get_recipe_info(recipe_str);
				check_empty(recipes[builder_type]);
				recipes[builder_type] = recipe;
			}

			validate();
		}

		ID get_id() const { return id; }

		bool can_build(const RockTypeMap<int>& resource_amounts, RockType target_type) const
		{
			auto can_build_idx = [&resource_amounts,&recipe=recipes[target_type]](RockType type)
			{
				return recipe[type] <= resource_amounts[type];
			};
			const utils::int_range range{ num_rock_types() };
			const bool result = std::all_of(ROCK_TYPE_ARRAY.begin(), ROCK_TYPE_ARRAY.end(), can_build_idx);
			return result;
		}

		Recipe get_recipe(RockType type) const
		{
			return recipes[type];
		}
	};

	auto begin(const Blueprint& bp) { return bp.begin(); }
	auto end(const Blueprint& bp) { return bp.end(); }

	int8_t get_most_bots_required(const Blueprint& blueprint, RockType bot_type)
	{
		if (bot_type == TARGET_ROCK)
		{
			return std::numeric_limits<int8_t>::max();
		}

		auto num_required = [bot_type](const Recipe& recipe)
		{
			return recipe[bot_type];
		};
		const Recipe result = utils::ranges::max_transform(blueprint, num_required);
		return result[bot_type];
	}

	int mine_rock(const Blueprint& blueprint, const MiningState& initial_state, int time_to_mine_for)
	{
		const RockTypeMap<int8_t> most_bots_required = [&blueprint]()
		{
			RockTypeMap<int8_t> result;
			std::transform(begin(ROCK_TYPE_ARRAY), end(ROCK_TYPE_ARRAY), begin(result),
				[&blueprint](RockType bot_type)
				{
					return get_most_bots_required(blueprint, bot_type);
				});
			return result;
		}();
		std::vector<MiningState> current_states{ initial_state };
		std::vector<MiningState> next_states;
		for (auto min : utils::int_range{ time_to_mine_for })
		{
			std::size_t num_skipped = 0;
			for (const MiningState& state : current_states)
			{
				MiningState next_state = state;
				for (RockType type : ROCK_TYPE_ARRAY)
				{
					next_state.resources[type] += next_state.num_bots[type];
				}

				auto try_add_state = [&next_states, &num_skipped](const MiningState& state_to_push)
				{
					auto new_state_is_dominated = [&state_to_push](const MiningState& other)
					{
						return other.dominates(state_to_push);
					};

					const bool is_dominated = std::ranges::any_of(next_states, new_state_is_dominated);
					if (is_dominated)
					{
						++num_skipped;
						return;
					}

					auto new_state_dominates = [&state_to_push](const MiningState& other)
					{
						return state_to_push.dominates(other);
					};

					num_skipped += utils::swap_remove_if(next_states, new_state_dominates);
					next_states.push_back(state_to_push);
				};

				for (RockType type : ROCK_TYPE_ARRAY)
				{
					if (!blueprint.can_build(state.resources, type)) continue;

					MiningState state_to_push = next_state;
					state_to_push.resources -= blueprint.get_recipe(type);
					const int new_num_bots = ++state_to_push.num_bots[type];

					// We can only build one bot per turn, so no point getting more bots than necessary.
					if ((type != TARGET_ROCK) && (new_num_bots > most_bots_required[type]))
					{
						++num_skipped;
						continue;
					}

					try_add_state(state_to_push);
				}

				try_add_state(next_state);
			}
			
			//std::cout << "\nMinute " << min + 1 << " Checked " << current_states.size() << " nodes. " << next_states.size() << " to check. Skipped " << num_skipped;

			std::swap(current_states, next_states);
			next_states.clear();
		}

		const MiningState result = utils::ranges::max_transform(current_states,
			[](const MiningState& s) {return s.resources[TARGET_ROCK]; });
		return result.resources[TARGET_ROCK];
	}

	int get_num_geodes_mined(const Blueprint& blueprint, int time_to_mine_for)
	{
		MiningState initial_state;
		initial_state.num_bots[RockType::ore] = 1;
		const int geodes_mined = mine_rock(blueprint, initial_state, time_to_mine_for);
		return geodes_mined;
	}

	Quality get_blueprint_quality(const Blueprint& blueprint, int time_to_mine_for)
	{
		const int geodes_mined = get_num_geodes_mined(blueprint, time_to_mine_for);
		const ID id = blueprint.get_id();
		const Quality quality = geodes_mined * id;
		//std::cout << "\nBlueprint " << id << " got " << geodes_mined << " geodes. Quality=" << quality;
		return quality;
	}

	ResultType solve_generic(std::istream& input, int time_to_mine_for, int num_blueprints, int init_value, auto eval_func, auto combo_func)
	{
		using ILI = utils::istream_line_iterator;
		using FutureType = std::future<int>;
		ILI it{ input };
		std::vector<FutureType> async_results;

		auto func = [time_to_mine_for, &eval_func](Blueprint blueprint) -> int
		{
			return eval_func(blueprint, time_to_mine_for);
		};

		for (int i = 0; i < num_blueprints && it != ILI{}; ++i, ++it)
		{
			FutureType fut = std::async(std::launch::async, func, Blueprint{ *it });
			async_results.emplace_back(std::move(fut));
		}

		const int result = std::transform_reduce(begin(async_results), end(async_results), init_value,
			combo_func, [](FutureType& ft) { return ft.get(); });

		return result;
	}

	ResultType solve_p1(std::istream& input)
	{
		constexpr int NUM_BLUEPRINTS = std::numeric_limits<int>::max();
		constexpr int TIME_TO_MINE_FOR = 24;
		return solve_generic(input, TIME_TO_MINE_FOR, NUM_BLUEPRINTS, 0, get_blueprint_quality, std::plus<int>{});
	}

	ResultType solve_p2(std::istream& input, auto combo_func)
	{
		constexpr int NUM_BLUEPRINTS = 3;
		constexpr int TIME_TO_MINE_FOR = 32;
		return solve_generic(input, TIME_TO_MINE_FOR, NUM_BLUEPRINTS, 1, get_num_geodes_mined, combo_func);
	}
}

ResultType day_nineteen_p1_a()
{
	auto input = advent::open_testcase_input(19, 'a');
	return solve_p1(input);
}

ResultType day_nineteen_p2_a()
{
	auto input = advent::open_testcase_input(19, 'a');
	return solve_p2(input, std::greater<int>{});
}

ResultType advent_nineteen_p1()
{
	auto input = advent::open_puzzle_input(19);
	return solve_p1(input);
}

ResultType advent_nineteen_p2()
{
	auto input = advent::open_puzzle_input(19);
	return solve_p2(input, std::multiplies<int>{});
}

#undef DAY19DBG
#undef ENABLE_DAY19DBG