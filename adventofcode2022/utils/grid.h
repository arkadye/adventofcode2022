#pragma once

#include <vector>
#include <iosfwd>

#include "../advent/advent_assert.h"
#include "istream_line_iterator.h"
#include "coords.h"

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
			for (int x = 0; x < x_max; ++x)
			{
				for (int y = 0; y < y_max; ++y)
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
	};

	std::vector<std::pair<int,int>> get_path(const utils::coords& start,
		const auto& is_end_fn /* Takes a unary node */,
		const auto& traverse_cost_fn /*  Takes fn(const utils::coords&,const utils::coords&); Return a cost type and a bool. If the bool = false, no traversal is possible. Defaults to {1.0f , true }. */,
		const auto& heuristic_fn /* Takes fn(const utils::coords&); Returns heuristic cost of reaching a node. */);

	std::vector<std::pair<int, int>> get_path(const utils::coords& start, const utils::coords& end,
		const auto& traverse_cost_fn /*  Takes fn(const utils::coords&,const utils::coords&); Return a cost type and a bool. If the bool = false, no traversal is possible. Defaults to {1.0f , true }. */,
		const auto& heuristic_fn /* Takes fn(const utils::coords&); Returns heuristic cost of reaching a node. */);

	std::vector<std::pair<int, int>> get_path(const utils::coords& start, const utils::coords& end,
		const auto& traverse_cost_fn /*  Takes fn(const utils::coords&,const utils::coords&); Return a cost type and a bool. If the bool = false, no traversal is possible. Defaults to {1.0f , true }. */);

	namespace grid
	{
		auto build(std::istream& iss, const auto& char_to_node_fn)
		{
			using NodeType = decltype(char_to_node_fn(' '));
			Grid<NodeType> result;
			result.build_from_stream(iss, char_to_node_fn);
			return result;
		}

		template <typename CostType>
		struct DefaultHeuristicFunctor
		{
		private:
			std::optional<utils::coords> target;
		public:
			DefaultHeuristicFunctor() {}
			explicit DefaultHeuristicFunctor(utils::coords target_init) : target{target_init}{}
			CostType operator()(utils::coords coords) const
			{
				return static_cast<CostType>(target.has_value() ? coords.manhatten_distance(target) : 0);
			}
		};

		template <typename CostType, int max_range, bool allow_diagonal>
		struct DefaultCostFunctor
		{
		public:
			std::pair<CostType,bool> operator()(utils::coords from, utils::coords to) const
			{
				const utils::coords diff = to - from;
				const int distance = diff.manhatten_distance();
				if constexpr (allow_diagonal)
				{
					const bool allowed = std::max(diff.x,diff.y) <= max_range;
					return std::pair{ static_cast<CostType>(distance),allowed };
				}
				if constexpr (!allow_diagonal)
				{
					const bool allowed = distance <= max_range;
					return std::pair{ static_cast<CostType>(distance), allowed };
				}
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