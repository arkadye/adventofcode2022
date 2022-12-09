#include "advent8.h"
#include "../advent/advent_utils.h"

#define ENABLE_DAY8DBG 1
#ifdef NDEBUG
#define DAY8DBG 0
#else
#define DAY8DBG ENABLE_DAY8DBG
#endif

#if DAY8DBG
	#include <iostream>
#endif

namespace
{
#if DAY8DBG
	std::ostream & log = std::cout;
#else
	struct {	template <typename T> auto& operator<<(const T&) const noexcept { return *this; } } log;
#endif
}

#include "range_contains.h"
#include "int_range.h"
#include "comparisons.h"
#include "istream_line_iterator.h"
#include <vector>
#include <algorithm>

namespace
{
	constexpr char no_tree = ('0' - 1);
	struct Tree
	{
		char height = no_tree;
		explicit Tree(char c) : height{ c }
		{
			AdventCheck(utils::range_contains_inc(c, '0', '9'));
		}
		Tree() : height{ no_tree } {}
	};

	struct Node
	{
		Tree tree;
		bool is_visible = false;
		int visibility_rating = 0;
		explicit Node(char c) : tree{ c } {}
	};

	struct Grid
	{
	private:
		std::vector<Node> nodes;
		int width = 0;
		int height = 0;
		std::size_t get_idx(int x, int y) const
		{
			AdventCheck(utils::range_contains_exc(x,0,static_cast<int>(width)));
			AdventCheck(utils::range_contains_exc(y,0,static_cast<int>(height)));
			const std::size_t result = y * width + x;
			AdventCheck(result < nodes.size());
			return result;
		}

		void add_line(std::string_view line)
		{
			if (width == 0)
			{
				width = static_cast<int>(line.size());
				nodes.reserve(width * width);
			}
			AdventCheck(line.size() == width);
			std::ranges::transform(line, std::back_inserter(nodes), [](char c) {return Node{ c }; });
			++height;
		}
	public:
		int get_height() const { return height; }
		int get_width() const { return width; }
		Node& at(int x, int y)
		{
			const std::size_t result = get_idx(x, y);
			return nodes[result];
		}

		Node at(int x, int y) const
		{
			const std::size_t result = get_idx(x, y);
			return nodes[result];
		}

		explicit Grid(std::istream& input)
		{
			for (auto line : utils::istream_line_range{ input })
			{
				add_line(line);
			}
		}

		auto begin() { return nodes.begin(); }
		auto end() { return nodes.end(); }
		auto begin() const { return nodes.begin(); }
		auto end() const { return nodes.end(); }
	};

	struct AxisSearchParams
	{
		int start = 0;
		int finish = 0;
		int stride = 0;
	};

	struct RayTraceParams
	{
		AxisSearchParams x, y;
	};

	Grid mark_visible_trees(Grid grid, const RayTraceParams& params)
	{
		Tree tallest;
		for (int x = params.x.start, y = params.y.start; x != params.x.finish && y != params.y.finish; x += params.x.stride, y += params.y.stride)
		{
			Node& node = grid.at(x, y);
			if (node.tree.height > tallest.height)
			{
				node.is_visible = true;
				tallest = node.tree;
			}
		}
		return grid;
	}

	Grid mark_visible_trees(Grid grid)
	{
		const int width = grid.get_width();
		const int height = grid.get_height();
		for (int x : utils::int_range(width))
		{
			RayTraceParams params;
			params.x.start = x;
			params.x.finish = width;
			params.x.stride = 0;

			// Bottom to top
			params.y.start = 0;
			params.y.finish = height;
			params.y.stride = 1;
			grid = mark_visible_trees(std::move(grid), params);

			// Top to bottom
			params.y.start = height - 1;
			params.y.finish = -1;
			params.y.stride = -1;
			grid = mark_visible_trees(std::move(grid), params);
		}

		for (int y : utils::int_range(height))
		{
			RayTraceParams params;
			params.y.start = y;
			params.y.finish = height;
			params.y.stride = 0;

			// Left to right
			params.x.start = 0;
			params.x.finish = width;
			params.x.stride = 1;
			grid = mark_visible_trees(std::move(grid), params);

			// Right to left
			params.x.start = width - 1;
			params.x.finish = -1;
			params.x.stride = -1;
			grid = mark_visible_trees(std::move(grid), params);
		}
		return grid;
	}

	int solve_p1(std::istream& input)
	{
		Grid grid{ input };
		grid = mark_visible_trees(std::move(grid));
		const auto result = std::ranges::count_if(grid, [](Node n) {return n.is_visible; });
		return static_cast<int>(result);
	}
}

namespace
{
	int get_generic_visibility(const Grid& grid, Tree tree, const RayTraceParams& params)
	{
		int result = 0;
		const int x_s = params.x.start;
		const int x_e = params.x.finish;
		const int y_s = params.y.start;
		const int y_e = params.y.finish;
		for (int x = x_s, y = y_s; x != x_e && y != y_e; x += params.x.stride, y += params.y.stride)
		{
			const Node& node = grid.at(x, y);
			++result;
			if (node.tree.height >= tree.height)
			{
				break;
			}
		}
		return result;
	}

	int get_north_visibility(const Grid& grid, int x, int y, Tree tree)
	{
		RayTraceParams params;
		params.x.start = x;
		params.x.finish = grid.get_width();
		params.x.stride = 0;
		params.y.start = y + 1;
		params.y.finish = grid.get_height();
		params.y.stride = 1;
		const int result = get_generic_visibility(grid, tree, params);
		return result;
	}

	int get_south_visibility(const Grid& grid, int x, int y, Tree tree)
	{
		RayTraceParams params;
		params.x.start = x;
		params.x.finish = grid.get_width();
		params.x.stride = 0;
		params.y.start = y - 1;
		params.y.finish = -1;
		params.y.stride = -1;
		const int result = get_generic_visibility(grid, tree, params);
		return result;
	}

	int get_west_visibility(const Grid& grid, int x, int y, Tree tree)
	{
		RayTraceParams params;
		params.x.start = x - 1;
		params.x.finish = -1;
		params.x.stride = -1;
		params.y.start = y;
		params.y.finish = grid.get_height();
		params.y.stride = 0;
		const int result = get_generic_visibility(grid, tree, params);
		return result;
	}

	int get_east_visibility(const Grid& grid, int x, int y, Tree tree)
	{
		RayTraceParams params;
		params.x.start = x + 1;
		params.x.finish = grid.get_width();
		params.x.stride = 1;
		params.y.start = y;
		params.y.finish = grid.get_height();
		params.y.stride = 0;
		const int result = get_generic_visibility(grid, tree, params);
		return result;
	}

	Grid mark_node_visibility(Grid grid, int x, int y)
	{
		Node& node = grid.at(x, y);
		Tree tree = node.tree;
		node.visibility_rating = 1;
		node.visibility_rating *= get_north_visibility(std::move(grid), x, y, tree);
		node.visibility_rating *= get_east_visibility(std::move(grid), x, y, tree);
		node.visibility_rating *= get_south_visibility(std::move(grid), x, y, tree);
		node.visibility_rating *= get_west_visibility(std::move(grid), x, y, tree);
		return grid;
	}

	Grid mark_all_visibilities(Grid grid)
	{
		for (int x : utils::int_range(1, grid.get_width() - 1))
		{
			for (int y : utils::int_range(1, grid.get_height() - 1))
			{
				grid = mark_node_visibility(std::move(grid), x, y);
			}
		}
		return grid;
	}

	int solve_p2(std::istream& input)
	{
		Grid grid{ input };
		grid = mark_all_visibilities(std::move(grid));
		const Node& result = utils::ranges::max_transform(grid, [](const Node& n) {return n.visibility_rating; });
		return result.visibility_rating;
	}
}

namespace
{
	std::istringstream testcase_a()
	{
		return std::istringstream{
			"30373\n"
			"25512\n"
			"65332\n"
			"33549\n"
			"35390"
		};
	}
}

ResultType day_eight_p1_a()
{
	auto input = testcase_a();
	return solve_p1(input);
}

ResultType day_eight_p2_a()
{
	auto input = testcase_a();
	return solve_p2(input);
}

ResultType advent_eight_p1()
{
	auto input = advent::open_puzzle_input(8);
	return solve_p1(input);
}

ResultType advent_eight_p2()
{
	auto input = advent::open_puzzle_input(8);
	return solve_p2(input);
}

#undef DAY8DBG
#undef ENABLE_DAY8DBG