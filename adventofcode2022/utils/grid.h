#pragma once

#include <vector>
#include <iosfwd>
#include <type_traits>
#include <optional>
#include <algorithm>

#include "../advent/advent_assert.h"
#include "istream_line_iterator.h"
#include "coords.h"
#include "int_range.h"

namespace utils
{
	template <typename NodeType>
	class Grid
	{
		std::vector<NodeType> m_nodes;
		utils::coords max_point;
		std::size_t get_idx(int x, int y) const;
	public:
		bool is_on_grid(int x, int y) const;
		bool is_on_grid(utils::coords coords) const { return is_on_grid(coords.x,coords.y); }

		NodeType& at(int x, int y) { return m_nodes[get_idx(x,y)]; }
		const NodeType& at(int x, int y) { return m_nodes[get_idx(x,y)]; }
		NodeType& at(utils::coords coords) { return at(coords.x,coords.y); }
		const NodeType& at(utils::coords coords) const { return at(coords.x,coords.y); }

		// Get a node using a predicate
		std::optional<utils::coords> get_coordinates_by_predicate(auto predicate) const
		{
			for (int x : utils::int_range{x_max})
			{
				for(int y : utils::int_range{y_max})
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
				if(x_max == 0)
				{
					x_max = line.size();
					m_nodes.reserve(x_max * x_max);
				}
				AdventCheck(x_max == line.size());
				std::transform(begin(line),end(line),std::back_inserter(m_nodes),char_to_node_fn);
			}
		}

		std::vector<utils::coords> get_path(const utils::coords& start, const auto& is_end_fn,
			const auto& traverse_cost_fn,
			const auto& heuristic_fn) const;

		std::vector<utils::coords> get_path(const utils::coords& start, const auto& is_end_fn,
			const auto& cost_or_heuristic_fn) const;

		std::vector<utils::coords> get_path(const utils::coords& start, const auto& is_end_fn) const;

		std::vector<utils::coords> get_path(const utils::coords& start, const utils::coords& end,
			const auto& traverse_cost_fn,
			const auto& heuristic_fn) const;

		std::vector<utils::coords> get_path(const utils::coords& start, const utils::coords& end,
			const auto& cost_or_heuristic_fn) const;

		std::vector<utils::coords> get_path(const utils::coords& start, const utils::coords& end) const;
	};

	namespace grid
	{
		auto build(std::istream& iss, const auto& char_to_node_fn)
		{
			using NodeType = decltype(char_to_node_fn(' '));
			Grid<NodeType> result;
			result.build_from_stream(iss, char_to_node_fn);
			return result;
		}

		struct DefaultHeuristicFunctor
		{
		private:
			std::optional<utils::coords> target;
		public:
			DefaultHeuristicFunctor() {}
			explicit DefaultHeuristicFunctor(utils::coords target_init) : target{target_init}{}
			float operator()(utils::coords coords) const
			{
				return static_cast<float>(target.has_value() ? coords.manhatten_distance(target.value()) : 0);
			}
		};

		template <bool allow_diagonal>
		struct DefaultCostFunctor
		{
		public:
			std::optional<float> operator()(utils::coords from, utils::coords to) const
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
						const result = val * val;
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
inline bool utils::Grid<NodeType>::is_on_grid(int x, int y) const
{
	if(x < 0) return false;
	if(y < 0) return false;
	if(x >= max_point.x) return false;
	if(y >= max_point.y) return false;
	return true;
}

template <typename NodeType>
inline std::size_t utils::Grid<NodeType>::get_idx(int x, int y) const
{
	AdventCheck(is_on_grid(x,y));
	const std::size_t result = x_max * y + x;
	return result;
}

template<typename NodeType>
inline std::vector<utils::coords> utils::Grid<NodeType>::get_path(const utils::coords& start, const auto& is_end_fn, const auto& traverse_cost_fn, const auto& heuristic_fn) const
{
	AdventCheck(is_on_grid(start));
	constexpr bool check_end_fn = std::is_invocable_r_v<bool, is_end_fn, utils::coords>;
	constexpr bool check_traverse_fn = std::is_invocable_r_v<std::optional<float>, traverse_cost_fn,, utils::coords, utils::coords>;
	constexpr bool check_heuristic_fn = std::is_invocable_r_v<float, heuristic_fn, utils::coords>;
	static_assert(check_end_fn, "is_end_fn must have the signature bool(utils::coords)");
	static_assert(check_traverse_fn, "traverse_fn must have the signature std::optional<float>(utils::coords,utils::coords)");
	static_assert(check_heuristic_fn, "heuristic_fn must have the signature float(utils::coords)");

	std::vector<utils::coords> result;

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

	std::vector<SearchNode> searched_nodes;
	std::vector<SearchNode> unsearched_nodes;

	auto try_add_node = [&unsearched_nodes,&searched_nodes, &traverse_cost_fn, &heuristic_fn]
		(int previous_node_id, const SearchNode& from, utils::coords to)
	{
		const auto has_checked_result = std::find_if(begin(searched_nodes), end(searched_nodes), [&to](const SearchNode& node)
			{
				return node.position == to;
			});
		if (has_checked_result != end(searched_nodes))
		{
			// We already checked this.
			return;
		}
		SearchNode result;
		result.previous_node_id = previous_node_id;
		result.position = to;
		if (previous_node_id >= 0)
		{
			const std::optional<float> latest_cost_opt = traverse_cost_fn(from.position, to);
			if (!latest_cost_opt.has_value())
			{
				return;
			}
			const float latest_cost = *latest_cost_opt;
			result.cost = from.cost + latest_cost;

			const float heuristic = heuristic_fn(to);
			result.cost_and_heuristic = result.cost + heuristic;
		}
		else
		{
			result.cost = 0.0f;
			result.cost_and_heuristic = 0.0f;
		}
		unsearched_nodes.push_back(result);
	};

	try_add_node(-1, start, start);

	while (!unsearched_nodes.empty())
	{
		const SearchNode next_node = unsearched_nodes.back();
		unsearched_nodes.pop_back();

		const bool node_is_end = is_end_fn(next_node.position);
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
			break;
		}

		const int previous_id = static_cast<int>(searched_nodes.size());
		for (int dx : utils::int_range{ -1,2 })
		{
			for (int dy : utils::int_range{ -1,2 })
			{
				const utils::coords delta_pos{ dx,dy };
				const utils::coords next_coords = next_node.position + delta_pos;
				try_add_node(previous_id, next_node, next_coords);
			}
		}

		std::sort(begin(unsearched_nodes), end(unsearched_nodes), order_on_heuristic);
	}

	return result;
}

template<typename NodeType>
inline std::vector<utils::coords> utils::Grid<NodeType>::get_path(const utils::coords& start, const auto& is_end_fn, const auto& cost_or_heuristic_fn) const
{
	constexpr bool is_cost_fn = std::is_invocable_v<std::optional<float>, cost_or_heuristic_fn,utils::coords, utils::coords>;
	constexpr bool is_heuristic_fn = std::is_invocable_r_v<float, cost_or_heuristic_fn, utils::coords>;
	static_assert(is_cost_fn || is_heuristic_fn, "cost_or_heuristic_fn must be a cost [std::optional<float>(utils::coords,utils::coords)] or a heuristic [float(utils::coords)] function");
	if constexpr (is_cost_fn)
	{
		auto heuristic_fn = utils::grid::DefaultHeuristicFunctor{};
		return get_path(start, is_end_fn, cost_or_heuristic_fn, heuristic_fn);
	}
	if constexpr (is_heuristic_fn)
	{
		auto cost_fn = utils::grid::DefaultCostFunctor<false>{};
		return get_path(start, is_end_fn, cost_fn, cost_or_heuristic_fn);
	}
	AdventUnreachable();
	return std::vector<utils::coords>{};
}

template<typename NodeType>
inline std::vector<utils::coords> utils::Grid<NodeType>::get_path(const utils::coords& start, const auto& is_end_fn) const
{
	return get_path(start, is_end_fn, utils::grid::DefaultCostFunctor<false>{}, utils::grid::DefaultHeuristicFunctor{});
}

template<typename NodeType>
inline std::vector<utils::coords> utils::Grid<NodeType>::get_path(const utils::coords& start, const utils::coords& end, const auto& traverse_cost_fn, const auto& heuristic_fn) const
{
	auto is_end_fn = [&end](const utils::coords& test)
	{
		return test == end;
	};
	return get_path(start, is_end_fn, traverse_cost_fn, heuristic_fn);
}

template<typename NodeType>
inline std::vector<utils::coords> utils::Grid<NodeType>::get_path(const utils::coords& start, const utils::coords& end, const auto& cost_or_heuristic_fn) const
{
	constexpr bool is_cost_fn = std::is_invocable_v<std::optional<float>, cost_or_heuristic_fn, utils::coords, utils::coords>;
	constexpr bool is_heuristic_fn = std::is_invocable_r_v<float, cost_or_heuristic_fn, utils::coords>;
	static_assert(is_cost_fn || is_heuristic_fn, "cost_or_heuristic_fn must be a cost [std::optional<float>(utils::coords,utils::coords)] or a heuristic [float(utils::coords)] function");
	if constexpr (is_cost_fn)
	{
		auto heuristic_fn = utils::grid::DefaultHeuristicFunctor{end};
		return get_path(start, end, cost_or_heuristic_fn, heuristic_fn);
	}
	if constexpr (is_heuristic_fn)
	{
		auto cost_fn = utils::grid::DefaultCostFunctor<false>{};
		return get_path(start, end, cost_fn, cost_or_heuristic_fn);
	}
	AdventUnreachable();
	return std::vector<utils::coords>{};
}

template<typename NodeType>
inline std::vector<utils::coords> utils::Grid<NodeType>::get_path(const utils::coords& start, const utils::coords& end) const
{
	return get_path(start, end, utils::grid::DefaultCostFunctor{}, utils::grid::DefaultHeuristicFunctor{ end });
}
