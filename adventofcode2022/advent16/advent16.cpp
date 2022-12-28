#include "advent16.h"
#include "../advent/advent_utils.h"

#define ENABLE_DAY16DBG 1
#ifdef NDEBUG
#define DAY16DBG 0
#else
#define DAY16DBG ENABLE_DAY16DBG
#endif

#if DAY16DBG
	#include <iostream>
#endif

namespace
{
#if DAY16DBG
	std::ostream & log = std::cout;
#else
	struct {	template <typename T> auto& operator<<(const T&) const noexcept { return *this; } } log;
#endif
}

#include "small_vector.h"
#include "sorted_vector.h"
#include "parse_utils.h"
#include "to_value.h"
#include "istream_line_iterator.h"
#include "swap_remove.h"
#include "transform_if.h"
#include "int_range.h"

#include <format>

namespace
{
	using FlowRate = int;
	using FlowTotal = int;

	struct ValveId
	{
#if DAY16DBG
		std::string val;
		explicit ValveId(std::string_view str) : val(str)
		{
			AdventCheck(str.size() == 2u || str.empty());
		}
#else
		uint16_t val;
		explicit ValveId(std::string_view str)
		{
			AdventCheck(str.size() <= 2u);
			val = 0;
			for (char c : str)
			{
				AdventCheck(c >= 0);
				val = val << CHAR_BIT;
				val += static_cast<uint16_t>(c);
			}
		}
#endif
		ValveId() : ValveId("") {}
		auto operator<=>(const ValveId&) const = default;
	};

	struct Valve
	{
		ValveId id;
		FlowRate flow_rate = 0;
		bool can_open() const { return flow_rate > 0; }
		constexpr auto operator<=>(const Valve& other) const
		{
			return id <=> other.id;
		}
	};

	struct Tunnel
	{
		ValveId to_valve;
		int time;
		explicit Tunnel(std::string_view sv) : to_valve{ sv }, time{ 1 } {}
		Tunnel() : Tunnel{ "" } {}
		constexpr auto operator<=>(const Tunnel& other) const
		{
			return to_valve <=> other.to_valve;
		}
		constexpr bool operator==(const Tunnel& other) const
		{
			const std::weak_ordering result = (*this <=> other);
			return result == std::weak_ordering::equivalent;
		}
	};

	struct Location
	{
		Valve valve;
		utils::small_vector<Tunnel, 5> tunnels;
		constexpr auto operator<=>(const Location& other) const
		{
			return valve <=> other.valve;
		}
	};

	Location parse_location(std::string_view line)
	{
		auto remove_prefix = [&line](std::string_view prefix)
		{
			line = utils::remove_specific_prefix(line,prefix);
		};
		Location result;
		remove_prefix("Valve ");
		{
			const auto [valve, rest_of_line] = utils::split_string_at_first(line, ' ');
			result.valve.id = ValveId{ valve };
			line = rest_of_line;
		}

		remove_prefix("has flow rate=");
		{
			const auto [flow_rate, rest_of_line] = utils::split_string_at_first(line, ';');
			result.valve.flow_rate = utils::to_value<FlowRate>(flow_rate);
			line = rest_of_line;
		}

		remove_prefix(" tunnel");
		if (line.front() == 's')
		{
			remove_prefix("s");
		}
		
		remove_prefix(" lead");
		if (line.front() == 's')
		{
			remove_prefix("s");
		}
		remove_prefix(" to valve");
		if (line.front() == 's')
		{
			remove_prefix("s");
		}

		while (!line.empty())
		{
			auto [next_valve_arg, rest_of_line] = utils::split_string_at_first(line, ',');
			next_valve_arg = utils::trim_string(next_valve_arg);
			Tunnel next_valve{ next_valve_arg };
			result.tunnels.push_back(std::move(next_valve));
			line = rest_of_line;
		}
		return result;
	}

	using ValveMap = utils::sorted_vector<Location>;

	ValveMap parse_all_locations(std::istream& input)
	{
		using ILI = utils::istream_line_iterator;
		ValveMap result;
		std::transform(ILI{ input }, ILI{}, std::back_inserter(result), parse_location);
		return result;
	}

	Location& get_location(ValveMap& valves, const ValveId& id)
	{
		const auto find_result = valves.binary_find_if(id, [](const Location& loc, const ValveId& id)
			{
				return loc.valve.id <=> id;
			});

		AdventCheck(find_result != end(valves));
		return *find_result;
	}

	ValveMap simplify_valve_map(ValveMap input, ValveId starting_valve)
	{
		auto should_keep_location = [starting_valve](const Location& loc)
		{
			return loc.valve.can_open() || loc.valve.id == starting_valve;
		};

		auto should_remove_location = [&should_keep_location](const Location& loc)
		{
			return !should_keep_location(loc);
		};

		while (true)
		{
			const auto loc_to_remove_it = input.find_if(should_remove_location);
			if (loc_to_remove_it == end(input)) break;

			const Location& loc_to_remove = *loc_to_remove_it;
			for (const Tunnel& tunnel_back : loc_to_remove.tunnels)
			{
				Location& other_location = get_location(input, tunnel_back.to_valve);
				{
					const auto tunnel_back_it = std::find_if(begin(other_location.tunnels), end(other_location.tunnels),
						[&loc_to_remove](const Tunnel& t)
						{
							return t.to_valve == loc_to_remove.valve.id;
						});
					AdventCheck(tunnel_back_it->time == tunnel_back.time);
					other_location.tunnels.erase(tunnel_back_it);
				}

				for (const Tunnel& tunnel_forward : loc_to_remove.tunnels)
				{
					if (tunnel_forward.to_valve == tunnel_back.to_valve) continue;
					const int total_time = tunnel_back.time + tunnel_forward.time;
					const auto previous_path_it = std::find(begin(other_location.tunnels), end(other_location.tunnels), tunnel_forward);
					if (previous_path_it == end(other_location.tunnels)) [[likely]]
					{
						Tunnel new_tunnel = tunnel_forward;
						new_tunnel.time = total_time;
						other_location.tunnels.push_back(new_tunnel);
					}
					else
					{
						previous_path_it->time = std::min(previous_path_it->time, total_time);
					}
				}
			}
			input.erase(loc_to_remove_it);
		}
		return input;
	}

	const Location& get_location(const ValveMap& valves, const ValveId& id)
	{
		const auto find_result = valves.binary_find_if(id, [](const Location& loc, const ValveId& id)
			{
				return loc.valve.id <=> id;
			});

		AdventCheck(find_result != end(valves));
		return *find_result;
	}

	struct SearchParameters
	{
		ValveId current_location;
		std::vector<ValveId> opened_valves;
		int time_remaining;
		auto operator<=>(const SearchParameters& other) const
		{
			const std::strong_ordering loc_order = current_location <=> other.current_location;
			if (loc_order == std::strong_ordering::equal || loc_order == std::strong_ordering::equivalent)
			{
				return loc_order;
			}

			return opened_valves <=> other.opened_valves;
		}
		bool current_valve_is_open() const
		{
			const auto find_it = std::find(begin(opened_valves), end(opened_valves), current_location);
			return find_it != end(opened_valves);
		}
	};
	struct SearchNode
	{
		utils::sorted_vector<ValveId> open_valves;
		ValveId current_location;
		int time_remaining = -1;
		FlowTotal current_flow = -1;
		FlowTotal best_possible_flow = -1;
		void set_best_possible_flow(const ValveMap& valves);
		bool is_end_point() const { return time_remaining <= 0 || open_valves.empty(); }
		bool is_valve_open(ValveId valve) const
		{
			return open_valves.contains(valve);
		}
	};

	FlowTotal calculate_best_possible_flow(const ValveMap& valves, const SearchNode& sn)
	{
		if (sn.is_end_point()) return sn.current_flow;
		const std::size_t num_valves_to_open = std::min(sn.open_valves.size(),static_cast<std::size_t>(sn.time_remaining / 2));
		utils::small_vector<FlowRate, 15> flows;
		std::transform(begin(sn.open_valves), end(sn.open_valves), std::back_inserter(flows),
			[&valves](const ValveId valve_id)
			{
				const Location& loc = get_location(valves, valve_id);
				const Valve& valve = loc.valve;
				const FlowRate flow = valve.flow_rate;
				return flow;
			});

		const auto mid_it = begin(flows) + num_valves_to_open;
		std::partial_sort(begin(flows), mid_it, end(flows), std::greater<FlowRate>{});
		flows.erase(mid_it, end(flows));

		FlowTotal result = sn.current_flow;
		auto flow_it = begin(flows);
		for (int mins = sn.time_remaining - 1; mins > 0; mins -= 2)
		{
			if (flow_it == end(flows)) break;
			const FlowRate this_flow = *(flow_it++);
			const FlowTotal this_total = this_flow * mins;
			result += this_total;
		}

		return result;
	}

	void SearchNode::set_best_possible_flow(const ValveMap& valves)
	{
		if (best_possible_flow >= 0) return;
		best_possible_flow = calculate_best_possible_flow(valves, *this);
	}

	void append_next_steps(std::vector<SearchNode>& out_nodes, const ValveMap& valves, SearchNode base_node)
	{
		const int original_time = base_node.time_remaining;
		const ValveId original_location_id = base_node.current_location;
		const Location& location = get_location(valves, original_location_id);

		auto append_base = [&out_nodes, &base_node,&valves]()
		{
			base_node.best_possible_flow = calculate_best_possible_flow(valves, base_node);
			out_nodes.push_back(base_node);
		};

		for (const Tunnel& to_next_location : location.tunnels)
		{
			base_node.current_location = to_next_location.to_valve;
			base_node.time_remaining = original_time - to_next_location.time;
			append_base();
		}

		base_node.current_location = original_location_id;

		if (base_node.is_valve_open(original_location_id))
		{
			const int time = original_time - 1;
			const FlowRate new_flow = location.valve.flow_rate;
			const FlowTotal extra_flow = new_flow * time;
			base_node.current_flow += extra_flow;
			base_node.open_valves.erase(original_location_id);
			base_node.time_remaining = time;
			append_base();
		}
	}

	FlowTotal get_best_possible_flow(const ValveMap& valves, ValveId starting_location,utils::sorted_vector<ValveId> valves_to_open, int starting_time, FlowTotal flow_cut_off)
	{
		std::vector<SearchNode> nodes_to_search;
		std::vector<SearchNode> searched_nodes;
		FlowTotal best_result_so_far = -1;
		{
			SearchNode initial_node;
			initial_node.current_flow = 0;
			initial_node.current_location = starting_location;
			initial_node.time_remaining = starting_time;
			initial_node.open_valves = std::move(valves_to_open);
			initial_node.set_best_possible_flow(valves);
			nodes_to_search.push_back(std::move(initial_node));
		}

		struct Stat
		{
#if DAY16DBG
		private:
			std::size_t val = 0;
		public:
			Stat& operator++() { ++val; return *this; }
			std::size_t operator()() const { return val; }
#else
			Stat& operator++() { return *this; }
			std::size_t operator()() const { return std::size_t{}; }
#endif
		};
		Stat stat_expanded;
		Stat stat_best_case_too_low;
		Stat stat_dominated;
		Stat stat_end_points_found;

		log << '\n';

		while (!nodes_to_search.empty())
		{
			log << std::format("\rBest: {} Searched: {} Unsearched: {} EPs: {} Dom: {} BCTL: {}",
				best_result_so_far,
				searched_nodes.size(),
				nodes_to_search.size(),
				stat_end_points_found(),
				stat_dominated(),
				stat_best_case_too_low());
			const auto best_match = std::max_element(begin(nodes_to_search), end(nodes_to_search),
				[](const SearchNode& left, const SearchNode& right)
				{
					if (left.current_flow != right.current_flow) [[likely]]
					{
						return left.current_flow < right.current_flow;
					}
					if (left.best_possible_flow != right.best_possible_flow) [[likely]]
					{
						return left.best_possible_flow < right.best_possible_flow;
					}
					return left.time_remaining < right.time_remaining;
				});

			const SearchNode current_node = *best_match;
			utils::swap_remove(nodes_to_search, best_match);

			best_result_so_far = std::max(best_result_so_far, current_node.current_flow);

			if (current_node.is_end_point())
			{
				++stat_end_points_found;
				continue;
			}

			if (current_node.best_possible_flow <= flow_cut_off)
			{
				++stat_best_case_too_low;
				continue;
			}

			if (current_node.best_possible_flow <= best_result_so_far)
			{
				++stat_best_case_too_low;
				continue;
			}

			auto is_previous_strictly_better = [&current_node](const SearchNode& previous_node)
			{
				if (previous_node.current_location != current_node.current_location) return false;
				if (previous_node.time_remaining < current_node.time_remaining) return false;
				if (previous_node.current_flow < current_node.current_flow) return false;
				if (std::any_of(begin(current_node.open_valves), end(current_node.open_valves),
					[&previous_node](ValveId valve)
					{
						return !previous_node.open_valves.contains(valve);
					}))
				{
					return false;
				}
				return true;
			};

			if (std::any_of(begin(searched_nodes), end(searched_nodes), is_previous_strictly_better))
			{
				++stat_dominated;
				continue;
			}

			++stat_expanded;
			append_next_steps(nodes_to_search, valves, current_node);

			searched_nodes.push_back(current_node);
		}
		log << '\n';
		return best_result_so_far;
	}

	template <std::size_t split_size>
	std::array<utils::sorted_vector<ValveId>, split_size> get_split_results(const utils::sorted_vector<ValveId>& valves)
	{
		static_assert(split_size >= 1u);
		if (split_size == 1u)
		{
			return { valves };
		}


	}

	template <AdventDay day>
	FlowTotal solve_generic(std::istream& input, std::string_view starting_location_str, int time)
	{
		const ValveId starting_location{ starting_location_str };
		const ValveMap valves = [&input, starting_location]()
		{
			ValveMap all_valves = parse_all_locations(input);
			const ValveMap result = simplify_valve_map(std::move(all_valves), starting_location);
			return result;
		}();

		utils::sorted_vector<ValveId> valves_to_open;
		utils::transform_if_pre(begin(valves), end(valves), std::back_inserter(valves_to_open),
			[](const Location& loc)
			{
				return loc.valve.id;
			},
			[](const Location& loc)
			{
				return loc.valve.can_open();
			});

		if constexpr (day == AdventDay::One)
		{
			const FlowTotal result = get_best_possible_flow(valves, starting_location, valves_to_open, time, 0);
			return result;
		}
		
		if constexpr (day == AdventDay::Two)
		{
			using MaskType = uint64_t;
			AdventCheck(valves_to_open.size() >= 1u);
			AdventCheck(valves_to_open.size() <= (sizeof(MaskType) * CHAR_BIT));
			FlowTotal best_result = 0;
			const MaskType split_mask_max = MaskType{ 1 } << (valves_to_open.size() - 1);
			utils::sorted_vector<ValveId> low_valves, high_valves;
			for (MaskType mask : utils::int_range{ split_mask_max })
			{
				low_valves.clear();
				high_valves.clear();
				for (auto idx : utils::int_range{ valves_to_open.size() })
				{
					const MaskType bit = MaskType{ 1u } << idx;
					const MaskType mask_bit = mask & bit;
					const bool is_high = mask_bit != 0;
					utils::sorted_vector<ValveId>& valves_to_add_to = (is_high ? high_valves : low_valves);
					valves_to_add_to.push_back(valves_to_open[idx]);
				}
				const bool low_is_longer = low_valves.size() > high_valves.size();
				const utils::sorted_vector<ValveId>& longer = low_is_longer ? low_valves : high_valves;
				const utils::sorted_vector<ValveId>& shorter = low_is_longer ? high_valves : low_valves;
				const FlowTotal long_result = get_best_possible_flow(valves, starting_location, longer, time, 0);
				const FlowTotal cut_off = best_result - long_result;
				const FlowTotal short_result = get_best_possible_flow(valves, starting_location, shorter, time, cut_off);
				const FlowTotal this_result = long_result + short_result;
				best_result = std::max(best_result, this_result);
			}
			return best_result;
		}
		AdventUnreachable();
		return 0;
	}

	FlowTotal solve_p1(std::istream& input)
	{
		return solve_generic<AdventDay::One>(input, "AA", 30);
	}

	int solve_p2(std::istream& input)
	{
		return solve_generic<AdventDay::Two>(input, "AA", 26);
	}
}

ResultType day_sixteen_p1_a()
{
	auto input = advent::open_testcase_input(16, 'a');
	return solve_p1(input);
}

ResultType day_sixteen_p2_a()
{
	auto input = advent::open_testcase_input(16, 'a');
	return solve_p2(input);
}

ResultType advent_sixteen_p1()
{
	auto input = advent::open_puzzle_input(16);
	return solve_p1(input);
}

ResultType advent_sixteen_p2()
{
	auto input = advent::open_puzzle_input(16);
	return solve_p2(input);
}

#undef DAY16DBG
#undef ENABLE_DAY16DBG