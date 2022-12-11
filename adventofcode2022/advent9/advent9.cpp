#include "advent9.h"
#include "../advent/advent_utils.h"

#define ENABLE_DAY9DBG 1
#ifdef NDEBUG
#define DAY9DBG 0
#else
#define DAY9DBG ENABLE_DAY9DBG
#endif

#if DAY9DBG
	#include <iostream>
#endif

namespace
{
#if DAY9DBG
	std::ostream & log = std::cout;
#else
	struct {	template <typename T> auto& operator<<(const T&) const noexcept { return *this; } } log;
#endif
}

#include "coords.h"
#include "int_range.h"
#include "to_value.h"
#include "istream_line_iterator.h"
#include "split_string.h"
#include "sorted_vector.h"

#include <algorithm>
#include <string_view>
#include <array>

namespace
{
	using utils::coords;
	using utils::direction;

	int get_vertical_distance(const coords& front, const coords& back)
	{
		return std::abs(front.x - back.x);
	}

	int get_horizontal_distance(const coords& front, const coords& back)
	{
		return std::abs(front.y - back.y);
	}

	int get_distance(const coords& front, const coords& back)
	{
		const int v = get_vertical_distance(front,back);
		const int h = get_horizontal_distance(front,back);
		return std::max(h, v);
	}

	template <int NumSegments>
	struct Rope
	{
	private:
		using Data_t = std::array<coords, NumSegments>;
		using It_t = Data_t::iterator;
		Data_t positions;
		static void resolve_tail(coords target, It_t front, It_t back)
		{
			for (It_t it = front; it != back; ++it)
			{
				const coords& front_pos = *it;
				const int distance = get_distance(target, front_pos);
				if (distance <= 1) return;

				coords move_direction = target - front_pos;
				auto normalize_axis = [](int& val)
				{
					val = std::clamp(val, -1, +1);
				};
				normalize_axis(move_direction.x);
				normalize_axis(move_direction.y);

				const coords new_pos = front_pos + move_direction;
				AdventCheck(get_distance(target, new_pos) <= 1);

				*it = target = new_pos;
			}
		}
	public:
		coords get_head() const { return positions.front(); }
		coords get_tail() const { return positions.back(); }

		void move_head(direction dir)
		{
			const coords head_offset = coords::dir(dir);
			positions.front() += head_offset;

			resolve_tail(get_head(), begin(positions) + 1, end(positions));
		}
	};

	using ParsedLine = std::pair<direction, int>;
	
	direction to_dir(char c)
	{
		switch (c)
		{
		case 'U':
			return direction::up;
		case 'R':
			return direction::right;
		case 'D':
			return direction::down;
		case 'L':
			return direction::left;
		default:
			AdventUnreachable();
			break;
		}
		return direction::up;
	}

	direction to_dir(std::string_view d)
	{
		AdventCheck(d.size() == std::size_t{ 1 });
		return to_dir(d[0]);
	}

	ParsedLine parse_line(std::string_view line)
	{
		const auto [dir_str, num_str] = utils::split_string_at_first(line, ' ');
		const direction dir = to_dir(dir_str);
		const int num = utils::to_value<int>(num_str);
		return ParsedLine{ dir,num };
	}

	template <int NumSegments>
	utils::sorted_vector<coords> track_tail(std::istream& input)
	{
		Rope<NumSegments> rope;
		utils::sorted_vector<coords> tail_coords;
		tail_coords.push_back(rope.get_tail());
		for (auto line : utils::istream_line_range{ input })
		{
			const ParsedLine parsed_line = parse_line(line);
			for (int i : utils::int_range{ parsed_line.second })
			{
				rope.move_head(parsed_line.first);
				tail_coords.push_back(rope.get_tail());
			}
		}

		tail_coords.unique();
		return tail_coords;
	}

	template <int NumSegments>
	int solve_generic(std::istream& input)
	{
		const auto tail_locations = track_tail<NumSegments>(input);
		const std::size_t result = tail_locations.size();
		return static_cast<int>(result);
	}

	int solve_p1(std::istream& input)
	{
		return solve_generic<2>(input);
	}

	int solve_p2(std::istream& input)
	{
		return solve_generic<10>(input);
	}
}

namespace
{
	std::istringstream testcase_a()
	{
		return std::istringstream{
			"R 4\n"
			"U 4\n"
			"L 3\n"
			"D 1\n"
			"R 4\n"
			"D 1\n"
			"L 5\n"
			"R 2"
		};
	}

	std::istringstream testcase_b()
	{
		return std::istringstream{
			"R 5\n"
			"U 8\n"
			"L 8\n"
			"D 3\n"
			"R 17\n"
			"D 10\n"
			"L 25\n"
			"U 20"
		};
	}
}

ResultType day_nine_p1_a()
{
	auto input = testcase_a();
	return solve_p1(input);
}

ResultType day_nine_p2_a()
{
	auto input = testcase_a();
	return solve_p2(input);
}

ResultType day_nine_p2_b()
{
	auto input = testcase_b();
	return solve_p2(input);
}

ResultType advent_nine_p1()
{
	auto input = advent::open_puzzle_input(9);
	return solve_p1(input);
}

ResultType advent_nine_p2()
{
	auto input = advent::open_puzzle_input(9);
	return solve_p2(input);
}

#undef DAY9DBG
#undef ENABLE_DAY9DBG