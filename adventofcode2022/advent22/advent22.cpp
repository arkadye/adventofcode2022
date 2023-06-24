#include "advent22.h"
#include "../advent/advent_utils.h"

#define ENABLE_DAY22DBG 1
#ifdef NDEBUG
#define DAY22DBG 0
#else
#define DAY22DBG ENABLE_DAY22DBG
#endif

#if DAY22DBG
	#include <iostream>
#endif

namespace
{
#if DAY22DBG
	std::ostream & log = std::cout;
#else
	struct {	template <typename T> auto& operator<<(const T&) const noexcept { return *this; } } log;
#endif
}

#include "coords.h"
#include "sorted_vector.h"
#include "range_contains.h"
#include "int_range.h"
#include "modular_int.h"

#include <vector>
#include <variant>

namespace
{
	using utils::coords;
	using utils::turn_dir;
	using utils::direction;

	constexpr char WALKABLE = '.';
	constexpr char WALL = '#';
	constexpr char OFF_MAP = ' ';

	using Move = std::variant<int, turn_dir>;
	using Path = std::vector<Move>;

	struct MovePrinter
	{
		MovePrinter(std::ostream& output) : os{ output } {}
		std::ostream& operator()(int val) { os << val << " steps"; return os; }
		std::ostream& operator()(turn_dir val)
		{
			switch (val)
			{
			case turn_dir::clockwise:
				os << "turn right";
				break;
			case turn_dir::anticlockwise:
				os << "turn left";
				break;
			default:
				AdventUnreachable();
				break;
			}
			return os;
		}
	private:
		std::ostream& os;
	};

	std::ostream& operator<<(std::ostream& os, Move move)
	{
		return std::visit(MovePrinter{os}, move);
	}

	// A row OR a column
	struct Line
	{
		utils::sorted_vector<int> walls;
		int first_walkable = std::numeric_limits<int>::max();
		int last_walkable = std::numeric_limits<int>::min();
		int get_num_walkable() const
		{
			return 1 + last_walkable - first_walkable;
		}
	};

	struct Map
	{
		std::vector<Line> columns;
		std::vector<Line> rows;
	};

	struct Position
	{
		coords location;
		direction heading;
	};

	std::ostream& operator<<(std::ostream& os, const Position& pos)
	{
		os << "Pos=[" << pos.location << "] heading=" << pos.heading;
		return os;
	}

	struct State
	{
		Map map;
		Position position;
	};

	Map parse_map(std::istream& input)
	{
		Map result;
		std::string line;
		for(int y=0;true;++y)
		{
			std::getline(input, line);
			if (line.empty())
			{
				break;
			}

			if (result.columns.size() < line.size())
			{
				result.columns.resize(line.size());
			}
			result.rows.emplace_back();
			for (int x = 0; x < std::ssize(line); ++x)
			{
				auto make_min = [](int& x, int y)
				{
					x = std::min(x, y);
				};

				auto make_max = [](int& x, int y)
				{
					x = std::max(x, y);
				};

				const char tile = line[x];
				switch (tile)
				{
				case WALL:
					result.columns[x].walls.insert(y);
					result.rows[y].walls.insert(x);
					[[fallthrough]];
				case WALKABLE:
					make_min(result.columns[x].first_walkable, y);
					make_max(result.columns[x].last_walkable, y);
					make_min(result.rows[y].first_walkable, x);
					make_max(result.rows[y].last_walkable, x);
					break;
				default:
					AdventCheck(tile == OFF_MAP);
					break;
				}
			}
		}
		return result;
	}

	std::istream& operator>>(std::istream& input, Move& result)
	{
		auto validate = [](int c)
		{
			return c == 'L' || c == 'R' || std::isdigit(c);
		};
		const int peek_result = input.peek();
		validate(peek_result);

		const char first_letter = static_cast<char>(peek_result);

		if (first_letter == 'L')
		{
			const int get_result = input.get();
			AdventCheck(get_result == 'L');
			result = turn_dir::anticlockwise;
		}
		else if (first_letter == 'R')
		{
			const int get_result = input.get();
			AdventCheck(get_result == 'R');
			result = turn_dir::clockwise;
		}
		else
		{
			AdventCheck(std::isdigit(first_letter));
			int num_to_go = 0;
			input >> num_to_go;
			result = num_to_go;
		}
		return input;
	}

	Path parse_path(std::istream& input)
	{
		using ItType = std::istream_iterator<Move>;
		Path result;
		while (!input.eof())
		{
			Move move;
			input >> move;
			result.push_back(move);
		}
		return result;
	}

	State parse_state(std::istream& input)
	{
		State result;
		result.map = parse_map(input);
		result.position.location.y = 0;
		result.position.location.x = result.map.rows[0].first_walkable;
		result.position.heading = direction::right;
		return result;
	}

	int get_password(const Position& position)
	{
		auto score_heading = [](direction dir)
		{
			switch (dir)
			{
			case direction::right:
				return 0;
			case direction::down:
				return 1;
			case direction::left:
				return 2;
			case direction::up:
				return 3;
			}
			AdventUnreachable();
			return -1;
		};

		const coords password_coords = position.location + coords{ 1,1 };
		return 1000 * password_coords.y + 4 * password_coords.x + score_heading(position.heading);
	}

	State make_move_turn(State state, turn_dir turn)
	{
		state.position.heading = utils::rotate(state.position.heading, turn);
		return state;
	}

	State make_move_walk(State state, int num_steps)
	{
		using utils::is_horizontal;
		const direction heading = state.position.heading;
		const Line& line = [&map=state.map,heading,&location=state.position.location]() -> const Line&
		{
			const Line& row = map.rows[location.y];
			const Line& column = map.columns[location.x];
			return is_horizontal(heading) ? row : column;
		}();

		int& changed_coord = is_horizontal(heading) ? state.position.location.x : state.position.location.y;
		const int start_coord = changed_coord;
		AdventCheck(utils::range_contains_inc(changed_coord, line.first_walkable, line.last_walkable));
		const int coord_delta = [heading, num_steps]()
		{
			switch (heading)
			{
			case direction::down:
			case direction::right:
				return +num_steps;
			case direction::up:
			case direction::left:
				return -num_steps;
			default:
				break;
			}
			AdventUnreachable();
			return 0;
		}();

		const int target_location = start_coord + coord_delta;
		if (line.walls.empty())
		{
			changed_coord = utils::get_unwound(target_location, line.first_walkable, line.last_walkable + 1);
			return state;
		}

		const bool is_going_forwards = coord_delta > 0;

		const int wall_stop_location = [&line, start_coord, is_going_forwards]()
		{
			const int line_len = line.get_num_walkable();
			auto forward_wall_it = line.walls.lower_bound(start_coord);
			AdventCheck(*forward_wall_it != start_coord);
			utils::modular wall_idx{ std::distance(begin(line.walls),forward_wall_it), std::ssize(line.walls)};
			if (!is_going_forwards) --wall_idx;
			int wall_location = line.walls[wall_idx];
			while (!is_going_forwards && wall_location > start_coord) wall_location -= line_len;
			while (is_going_forwards && wall_location < start_coord) wall_location += line_len;
			return wall_location + (is_going_forwards ? -1 : +1);
		}();

		const int final_location = [target_location, is_going_forwards, wall_stop_location]
		{
			if (is_going_forwards)
			{
				return std::min(target_location, wall_stop_location);
			}
			return std::max(target_location, wall_stop_location);
		}();

		changed_coord = utils::get_unwound(final_location, line.first_walkable, line.last_walkable + 1);
		return state;
	}

	struct MoveMaker
	{
		MoveMaker(State init) : state{ std::move(init) } {}
		State operator()(int steps)
		{
			return make_move_walk(std::move(state), steps);
		}
		State operator()(turn_dir turn)
		{
			return make_move_turn(std::move(state), turn);
		}
	private:
		State state;
	};

	State make_move(State state, Move move)
	{
		log << "\n    Start=[" << state.position << "] Move=[" << move;
		State result = std::visit(MoveMaker{ std::move(state) }, move);
		log << "] Finish=[" << result.position << ']';
		return result;
	}

	State follow_path(State state, const Path& path)
	{
		const State result = std::accumulate(begin(path), end(path), std::move(state), make_move);
		return result;
	}

	int solve_p1(std::istream& input)
	{
		State state = parse_state(input);
		const Path path = parse_path(input);
		const State result = follow_path(std::move(state), path);
		const int password = get_password(result.position);
		log << "\nFinal location=[" << result.position.location << "] "
			"heading=" << result.position.heading << 
			" password=" << password << '\n';
		return password;
	}

	int solve_p2(std::istream& input)
	{
		return 0;
	}
}

ResultType day_twentytwo_p1_a()
{
	auto input = advent::open_testcase_input(22, 'a');
	return solve_p1(input);
}

ResultType day_twentytwo_p2_a()
{
	auto input = advent::open_testcase_input(22, 'a');
	return solve_p2(input);
}

ResultType advent_twentytwo_p1()
{
	auto input = advent::open_puzzle_input(22);
	return solve_p1(input);
}

ResultType advent_twentytwo_p2()
{
	auto input = advent::open_puzzle_input(22);
	return solve_p2(input);
}

#undef DAY22DBG
#undef ENABLE_DAY22DBG