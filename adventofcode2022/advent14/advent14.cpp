#include "advent14.h"
#include "../advent/advent_utils.h"

#define ENABLE_DAY14DBG 0
#ifdef NDEBUG
#define DAY14DBG 0
#else
#define DAY14DBG ENABLE_DAY14DBG
#endif

#if DAY14DBG
	#include <iostream>
#endif

namespace
{
#if DAY14DBG
	std::ostream & log = std::cout;
#else
	struct {	template <typename T> auto& operator<<(const T&) const noexcept { return *this; } } log;
#endif
}

#include <map>
#include <optional>

#include "coords.h"
#include "comparisons.h"
#include "int_range.h"

namespace
{
	using utils::coords;
	enum class Block : char
	{
		wall = '#',
		space = ' ',
		sand = 'o',
		abyss = '-'
	};

	class Cave
	{
		std::map<coords, Block> m_data;
		int m_floor_level = INT_MIN;
		int m_highest_point = INT_MAX;
		Block m_floor_type = Block::abyss;
	public:
		Block get_block(coords c) const
		{
			if (c.y >= m_floor_level) return m_floor_type;
			const auto result = m_data.find(c);
			const bool found_it = result != end(m_data);
			return found_it ? result->second : Block::space;
		}

		void set_floor_type(Block new_floor_type)
		{
			m_floor_type = new_floor_type;
		}
		
		void set_block(coords c, Block b)
		{
			AdventCheck(b != Block::abyss);
			if (b == Block::space)
			{
				AdventCheckMsg(false, "Adding space blocks isn't supported. Use m_data.erase() and then resolve m_abyss_start.");
			}
			else
			{
				m_data.insert_or_assign(c, b);
				m_highest_point = std::min(m_highest_point, c.y);
				if (b != Block::sand)
				{
					m_floor_level = std::max(m_floor_level, c.y + 2);
				}
			}
		}

		void add_wall(coords start, coords finish)
		{
			const bool is_vertical = (start.x == finish.x);
			const bool is_horizontal = (start.y == finish.y);
			AdventCheck(is_vertical != is_horizontal);
			const int pos_start = is_horizontal ? start.x : start.y;
			const int pos_finish = is_horizontal ? finish.x : finish.y;
			const auto [range_low, range_high] = std::minmax(pos_start, pos_finish);
			const int static_coord = is_horizontal ? start.y : start.x;
			for (int i : utils::int_range{ range_low,range_high+1 })
			{
				coords wall_pos;
				(is_horizontal ? wall_pos.x : wall_pos.y) = i;
				(is_horizontal ? wall_pos.y : wall_pos.x) = static_coord;
				set_block(wall_pos, Block::wall);
			}
		}

		std::optional<coords> add_sand(int x_coordinate)
		{
			coords sand_position{ x_coordinate,m_highest_point-1 };
			while (sand_position.y < m_floor_level)
			{
				auto try_point = [this, &sand_position](coords next_position)
				{
					const Block b = get_block(next_position);
					switch (b)
					{
					case Block::wall:
					case Block::sand:
						return false;
					case Block::space:
					case Block::abyss:
						sand_position = next_position;
						return  true;
					default:break;
					}
					return false;
				};

				const bool fell = try_point(sand_position + coords::up())
					|| try_point(sand_position + coords::up() + coords::left())
					|| try_point(sand_position + coords::up() + coords::right());
				if (!fell)
				{
					set_block(sand_position, Block::sand);
					return sand_position;
				}
			}
			return std::nullopt;
		}

		void to_stream(auto& stream) const
		{
#if DAY14DBG
			AdventCheck(m_highest_point < m_floor_level);
			using ElemType = decltype(m_data)::value_type;
			const auto [min_x, max_x] = [&data = m_data]()
			{
				const auto [min_x_it, max_x_it] = utils::ranges::minmax_transform(data, [](ElemType elem) {return elem.first.x; });
				return std::pair{ min_x_it.first.x,max_x_it.first.x };
			}();
			AdventCheck(min_x < max_x);
			for (int y : utils::int_range{ m_highest_point,m_floor_level+1 })
			{
				stream << '\n';
				for (int x : utils::int_range{ min_x,max_x + 1,1 })
				{
					const Block b = get_block(coords{ x,y });
					const char c = static_cast<char>(b);
					stream << c;
				}
			}
			stream << '\n';
#endif
		}
	};

	Cave parse_caves(std::istream& iss, Block floor_type)
	{
		Cave result;
		result.set_floor_type(floor_type);
		constexpr std::string_view arrow = "->";
		bool just_read_an_arrow = false;
		coords previous_read;
		while (!iss.eof())
		{
			std::string token;
			iss >> token;
			if (token == arrow)
			{
				just_read_an_arrow = true;
			}
			else
			{
				coords latest_read = coords::from_chars(token);
				if (just_read_an_arrow)
				{
					result.add_wall(previous_read, latest_read);
					just_read_an_arrow = false;
				}
				previous_read = latest_read;
			}
		}
		result.to_stream(log);
		return result;
	}

	int fill_with_sand(Cave caves)
	{
		int result = 0;
		while (true)
		{
			const auto sand_pos = caves.add_sand(500);
			if (sand_pos.has_value())
			{
				++result;
				log << "Num bits of sand: " << result;
				caves.to_stream(log);
				if (sand_pos == coords{ 500,0 })
				{
					return result;
				}
			}
			else
			{
				return result;
			}
		}
		AdventUnreachable();
		return -1;
	}

	int solve_generic(std::istream& input, Block floor_type)
	{
		Cave caves = parse_caves(input, floor_type);
		const int result = fill_with_sand(std::move(caves));
		return result;
	}

	int solve_p1(std::istream& input)
	{
		return solve_generic(input, Block::abyss);
	}

	int solve_p2(std::istream& input)
	{
		return solve_generic(input, Block::wall);
	}
}

namespace
{
	std::istringstream testcase_a()
	{
		return std::istringstream{
			"498,4 -> 498,6 -> 496,6\n"
			"503,4 -> 502,4 -> 502,9 -> 494,9"
		};
	};
}

ResultType day_fourteen_p1_a()
{
	auto input = testcase_a();
	return solve_p1(input);
}

ResultType day_fourteen_p2_a()
{
	auto input = testcase_a();
	return solve_p2(input);
}

ResultType advent_fourteen_p1()
{
	auto input = advent::open_puzzle_input(14);
	return solve_p1(input);
}

ResultType advent_fourteen_p2()
{
	auto input = advent::open_puzzle_input(14);
	return solve_p2(input);
}

#undef DAY14DBG
#undef ENABLE_DAY14DBG