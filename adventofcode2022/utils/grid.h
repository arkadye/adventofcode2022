#pragma once

#include <iosfwd>
#include <type_traits>
#include <optional>
#include <algorithm>

#include "../advent/advent_assert.h"
#include "istream_line_iterator.h"
#include "coords.h"
#include "int_range.h"
#include "small_vector.h"

#define AOC_GRID_DEBUG_DEFAULT 0
#if NDEBUG
#define AOC_GRID_DEBUG 0
#else
#define AOC_GRID_DEBUG AOC_GRID_DEBUG_DEFAULT
#endif

#if AOC_GRID_DEBUG
#include <iostream>
#endif

namespace utils
{
	template <typename NodeType>
	class grid
	{
		utils::small_vector<NodeType,1> m_nodes;
		utils::coords max_point;
		std::size_t get_idx(int x, int y) const;
	public:
		bool is_on_grid(int x, int y) const;
		bool is_on_grid(utils::coords coords) const { return is_on_grid(coords.x,coords.y); }

		NodeType& at(int x, int y) { return m_nodes[get_idx(x,y)]; }
		const NodeType& at(int x, int y) const { return m_nodes[get_idx(x,y)]; }
		NodeType& at(utils::coords coords) { return at(coords.x,coords.y); }
		const NodeType& at(utils::coords coords) const { return at(coords.x,coords.y); }

		// Get all nodes that meet a predicate
		utils::small_vector<coords,1> get_all_coordinates_by_predicate(auto predicate) const
		{
			utils::small_vector<coords, 1> result;
			for (int x : utils::int_range{ max_point.x })
			{
				for (int y : utils::int_range{ max_point.y })
				{
					const NodeType& elem = at(x, y);
					if (predicate(elem))
					{
						result.push_back(utils::coords{ x,y });
					}
				}
			}
			return result;
		}

		// Get a node using a predicate
		std::optional<utils::coords> get_coordinates_by_predicate(auto predicate) const
		{
			for (int x : utils::int_range{max_point.x})
			{
				for(int y : utils::int_range{max_point.y})
				{
					const NodeType& elem = at(x,y);
					if (predicate(elem))
					{
						return utils::coords{x,y};
					}
				}
			}
			return std::nullopt;
		}

		// Get a node using NodeType::operator==
		std::optional<utils::coords> get_coordinates(const NodeType& node) const
		{
			return get_coordinates_by_predicate([&node](const NodeType& other)
			{
				return node == other;
			});
		}

		void build_from_stream(std::istream& iss, const auto& char_to_node_fn)
		{
			for (auto line : utils::istream_line_range{ iss })
			{
				if(max_point.x == 0)
				{
					max_point.x = static_cast<int>(line.size());
					m_nodes.reserve(max_point.x * max_point.x);
				}
				AdventCheck(max_point.x == static_cast<int>(line.size()));
				std::transform(begin(line),end(line),std::back_inserter(m_nodes),char_to_node_fn);
				++max_point.y;
			}
#if AOC_GRID_DEBUG
			std::cout << "Created grid with dimenstions [" << max_point << '\n';
#endif
		}

		utils::small_vector<utils::coords,1> get_path(const utils::coords& start, const auto& is_end_fn,
			const auto& traverse_cost_fn,
			const auto& heuristic_fn) const;

		utils::small_vector<utils::coords,1> get_path(const utils::coords& start, const auto& is_end_fn,
			const auto& cost_or_heuristic_fn) const;

		utils::small_vector<utils::coords,1> get_path(const utils::coords& start, const auto& is_end_fn) const;

		utils::small_vector<utils::coords,1> get_path(const utils::coords& start, const utils::coords& end,
			const auto& traverse_cost_fn,
			const auto& heuristic_fn) const;

		utils::small_vector<utils::coords,1> get_path(const utils::coords& start, const utils::coords& end,
			const auto& cost_or_heuristic_fn) const;

		utils::small_vector<utils::coords,1> get_path(const utils::coords& start, const utils::coords& end) const;
	};

	namespace grid_helpers
	{
		template <typename NodeType, typename FnType>
		constexpr bool is_end_fn()
		{
			return std::is_invocable_r_v<bool, FnType, utils::coords, NodeType>;
		}

		template <typename NodeType, typename FnType>
		constexpr bool is_cost_fn()
		{
			return std::is_invocable_r_v < std::optional<float>,
				FnType,
				utils::coords, NodeType,
				utils::coords, NodeType>;
		}

		template <typename NodeType, typename FnType>
		constexpr bool is_heuristic_fn()
		{
			return std::is_invocable_r_v<float, FnType, utils::coords, NodeType>;
		}

		auto build(std::istream& iss, const auto& char_to_node_fn)
		{
			using NodeType = decltype(char_to_node_fn(' '));
			grid<NodeType> result;
			result.build_from_stream(iss, char_to_node_fn);
			return result;
		}

		template <typename NodeType>
		struct DefaultHeuristicFunctor
		{
		private:
			std::optional<utils::coords> target;
		public:
			DefaultHeuristicFunctor() {}
			explicit DefaultHeuristicFunctor(utils::coords target_init) : target{target_init}{}
			float operator()(utils::coords coords, const NodeType& node) const
			{
				return static_cast<float>(target.has_value() ? coords.manhatten_distance(target.value()) : 0);
			}
		};

		template <typename NodeType, bool allow_diagonal>
		struct DefaultCostFunctor
		{
		public:
			std::optional<float> operator()(utils::coords from, const NodeType& from_node, utils::coords to, const NodeType& to_node) const
			{
				const utils::coords diff = to - from;
				const int distance = diff.manhatten_distance();
				if (distance == 0)
				{
					return std::nullopt;
				}
				if constexpr (allow_diagonal)
				{
					auto square = [](int val)
					{
						const int result = val * val;
						return static_cast<float>(result);
					};
					const bool allowed = std::max(std::abs(diff.x),std::abs(diff.y)) <= 1;
					if (allowed)
					{
						return std::sqrt(square(diff.x) + square(diff.y));
					}
				}
				if constexpr (!allow_diagonal)
				{
					const bool allowed = distance <= 1;
					if (allowed)
					{
						return static_cast<float>(distance);
					}
				}
				return std::nullopt;
			}
		};
	}
}

template <typename NodeType>
inline bool utils::grid<NodeType>::is_on_grid(int x, int y) const
{
	if(x < 0) return false;
	if(y < 0) return false;
	if(x >= max_point.x) return false;
	if(y >= max_point.y) return false;
	return true;
}

template <typename NodeType>
inline std::size_t utils::grid<NodeType>::get_idx(int x, int y) const
{
	AdventCheck(is_on_grid(x,y));
	const std::size_t result = max_point.x * y + x;
	return result;
}

template<typename NodeType>
inline utils::small_vector<utils::coords,1> utils::grid<NodeType>::get_path(const utils::coords& start, const auto& is_end_fn, const auto& traverse_cost_fn, const auto& heuristic_fn) const
{
	AdventCheck(is_on_grid(start));
	constexpr bool check_end_fn = utils::grid_helpers::is_end_fn<NodeType,decltype(is_end_fn)>();
	constexpr bool check_traverse_fn = utils::grid_helpers::is_cost_fn<NodeType,decltype(traverse_cost_fn)>();
	constexpr bool check_heuristic_fn = utils::grid_helpers::is_heuristic_fn<NodeType,decltype(heuristic_fn)>();
	static_assert(check_end_fn, "is_end_fn must have the signature bool(utils::coords,NodeType)");
	static_assert(check_traverse_fn, "traverse_fn must have the signature std::optional<float>(utils::coords,NodeType,utils::coords,NodeType)");
	static_assert(check_heuristic_fn, "heuristic_fn must have the signature float(utils::coords,NodeType)");

	utils::small_vector<utils::coords,1> result;

	struct SearchNode
	{
		int previous_node_id = -1;
		utils::coords position;
		float cost = 0.0f;
		float cost_and_heuristic = 0.0f;
	};

	auto order_on_heuristic = [](const SearchNode& left, const SearchNode& right)
	{
		return left.cost_and_heuristic > right.cost_and_heuristic;
	};

	utils::small_vector<SearchNode,1> searched_nodes;
	utils::small_vector<SearchNode,1> unsearched_nodes;

	auto try_add_node = [this,&unsearched_nodes,&searched_nodes, &traverse_cost_fn, &heuristic_fn]
		(int previous_node_id, const SearchNode& from, utils::coords to)
	{
		if (!is_on_grid(to))
		{
#if AOC_GRID_DEBUG
			std::cout << "    Skip adding node at " << to << ": Not on grid.\n";
#endif
			return;
		}
		const auto has_checked_result = std::find_if(begin(searched_nodes), end(searched_nodes), [this,&to](const SearchNode& node)
			{
				return node.position == to;
			});
		if (has_checked_result != end(searched_nodes))
		{
#if AOC_GRID_DEBUG
			std::cout << "    Skip adding node at " << to << ": Already checked.\n";
#endif
			return;
		}
		SearchNode result;
		result.previous_node_id = previous_node_id;
		result.position = to;

		const NodeType& to_node(at(to));
		if (previous_node_id >= 0)
		{
			const NodeType& from_node = at(from.position);
			const std::optional<float> latest_cost_opt = traverse_cost_fn(from.position, from_node, to, to_node);
			if (!latest_cost_opt.has_value())
			{
#if AOC_GRID_DEBUG
				std::cout << "    Skip adding node at " << to << ": Cannot traverse.\n";
#endif
				return;
			}
			const float latest_cost = *latest_cost_opt;
			result.cost = from.cost + latest_cost;
		}
		else
		{
			result.cost = 0.0f;
		}
		const float heuristic = heuristic_fn(to, to_node);
		result.cost_and_heuristic = result.cost + heuristic;
#if AOC_GRID_DEBUG
		std::cout << "    Adding node to search: Loc="
			<< result.position << " C=" << result.cost << " H=" << result.cost_and_heuristic << '\n';
#endif
		unsearched_nodes.push_back(result);
	};

	{
		SearchNode first_node;
		first_node.position = start;
		try_add_node(-1, SearchNode{}, start);
	}

	while (!unsearched_nodes.empty())
	{
#if AOC_GRID_DEBUG
		std::cout << "Searched nodes: " << searched_nodes.size() << " Unsearched nodes: " << unsearched_nodes.size() << '\n';
#endif
		const SearchNode next_node = unsearched_nodes.back();
#if AOC_GRID_DEBUG
		std::cout << "Expanding node: " << next_node.position << " with cost=" << next_node.cost
			<< " heuristic=" << next_node.cost_and_heuristic << '\n';
#endif
		unsearched_nodes.pop_back();

		{
			const auto already_checked_result = std::find_if(begin(searched_nodes), end(searched_nodes),
				[&next_node](const SearchNode& previous_node)
				{
					return next_node.position == previous_node.position;
				});
			if (already_checked_result != end(searched_nodes))
			{
#if AOC_GRID_DEBUG
				std::cout << "    Skipping node " << next_node.position << ": already searched here.\n";
#endif
				continue;
			}
		}

		const bool node_is_end = is_end_fn(next_node.position,at(next_node.position));
		if (node_is_end)
		{
			SearchNode path_node = next_node;
			while(true)
			{
				result.push_back(path_node.position);
				if (path_node.previous_node_id < 0)
				{
					break;
				}
				else
				{
					const std::size_t previous_idx = static_cast<std::size_t>(path_node.previous_node_id);
					AdventCheck(previous_idx < searched_nodes.size());
					path_node = searched_nodes[previous_idx];
				}
			}
#if AOC_GRID_DEBUG
			std::cout << "Found target node: " << next_node.position << " Total path len=" << result.size() << '\n';
#endif
			break;
		}

		const int previous_id = static_cast<int>(searched_nodes.size());
		for (int dx : utils::int_range{ -1,2 })
		{
			for (int dy : utils::int_range{ -1,2 })
			{
				if (dx == 0 && dy == 0) continue;
				const utils::coords delta_pos{ dx,dy };
				const utils::coords next_coords = next_node.position + delta_pos;
				try_add_node(previous_id, next_node, next_coords);
			}
		}

		std::sort(begin(unsearched_nodes), end(unsearched_nodes), order_on_heuristic);
		searched_nodes.push_back(next_node);
	}

	return result;
}

template<typename NodeType>
inline utils::small_vector<utils::coords,1> utils::grid<NodeType>::get_path(const utils::coords& start, const auto& is_end_fn, const auto& cost_or_heuristic_fn) const
{
	constexpr bool is_cost_fn = utils::grid_helpers::is_cost_fn<NodeType,decltype(cost_or_heuristic_fn)>();
	constexpr bool is_heuristic_fn = utils::grid_helpers::is_heuristic_fn<NodeType, decltype(cost_or_heuristic_fn)>();
	static_assert(is_cost_fn || is_heuristic_fn, "cost_or_heuristic_fn must be a cost [std::optional<float>(utils::coords,utils::coords)] or a heuristic [float(utils::coords)] function");
	if constexpr (is_cost_fn)
	{
		auto heuristic_fn = utils::grid_helpers::DefaultHeuristicFunctor<NodeType>{};
		return get_path(start, is_end_fn, cost_or_heuristic_fn, heuristic_fn);
	}
	if constexpr (is_heuristic_fn)
	{
		auto cost_fn = utils::grid_helpers::DefaultCostFunctor<false>{};
		return get_path(start, is_end_fn, cost_fn, cost_or_heuristic_fn);
	}
	AdventUnreachable();
	return utils::small_vector<utils::coords,1>{};
}

template<typename NodeType>
inline utils::small_vector<utils::coords,1> utils::grid<NodeType>::get_path(const utils::coords& start, const auto& is_end_fn) const
{
	return get_path(start, is_end_fn, utils::grid_helpers::DefaultCostFunctor<false>{}, utils::grid_helpers::DefaultHeuristicFunctor<NodeType>{});
}

template<typename NodeType>
inline utils::small_vector<utils::coords,1> utils::grid<NodeType>::get_path(const utils::coords& start, const utils::coords& end, const auto& traverse_cost_fn, const auto& heuristic_fn) const
{
	auto is_end_fn = [&end](const utils::coords& test, const NodeType& node)
	{
		return test == end;
	};
	return get_path(start, is_end_fn, traverse_cost_fn, heuristic_fn);
}

template<typename NodeType>
inline utils::small_vector<utils::coords,1> utils::grid<NodeType>::get_path(const utils::coords& start, const utils::coords& end, const auto& cost_or_heuristic_fn) const
{
	constexpr bool is_cost_fn = utils::grid_helpers::is_cost_fn<NodeType,decltype(cost_or_heuristic_fn)>();
	constexpr bool is_heuristic_fn = utils::grid_helpers::is_heuristic_fn<NodeType,decltype(cost_or_heuristic_fn)>();
	static_assert(is_cost_fn || is_heuristic_fn, "cost_or_heuristic_fn must be a cost [std::optional<float>(utils::coords,utils::coords)] or a heuristic [float(utils::coords)] function");
	if constexpr (is_cost_fn)
	{
		auto heuristic_fn = utils::grid_helpers::DefaultHeuristicFunctor<NodeType>{end};
		return get_path(start, end, cost_or_heuristic_fn, heuristic_fn);
	}
	if constexpr (is_heuristic_fn)
	{
		auto cost_fn = utils::grid_helpers::DefaultCostFunctor<false>{};
		return get_path(start, end, cost_fn, cost_or_heuristic_fn);
	}
	AdventUnreachable();
	return utils::small_vector<utils::coords,1>{};
}

template<typename NodeType>
inline utils::small_vector<utils::coords,1> utils::grid<NodeType>::get_path(const utils::coords& start, const utils::coords& end) const
{
	return get_path(start, end, utils::grid_helpers::DefaultCostFunctor{}, utils::grid_helpers::DefaultHeuristicFunctor<NodeType>{ end });
}
