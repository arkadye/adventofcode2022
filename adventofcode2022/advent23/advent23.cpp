#include "advent23.h"
#include "../advent/advent_utils.h"

#define ENABLE_DAY23DBG 1
#ifdef NDEBUG
#define DAY23DBG 0
#else
#define DAY23DBG ENABLE_DAY23DBG
#endif

#if DAY23DBG
	#include <iostream>
#endif

namespace
{
#if DAY23DBG
	std::ostream & log = std::cout;
#else
	struct {	template <typename T> auto& operator<<(const T&) const noexcept { return *this; } } log;
#endif
}

#include "coords.h"
#include <vector>
#include "sorted_vector.h"
#include <execution>
#include "int_range.h"
#include "range_contains.h"

namespace
{
	using Coords = utils::coords;
	using Dir = utils::direction;
	using ElfPositions = std::vector<Coords>;
	using AreaMap = utils::sorted_vector<Coords>;

	bool is_free(const AreaMap& map, const Coords& loc)
	{
		return !map.contains(loc);
	}

	constexpr char ELF_SYMBOL = '#';
	constexpr char EMPTY_SYMBOL = '.';

	using DirMask = uint_fast8_t;
	
	std::size_t get_dir_idx(Coords dir)
	{
		const auto directions = Coords{}.neighbours_plus_diag();
		const auto find_result = std::ranges::find(directions, dir);
		AdventCheck(find_result != end(directions));
		const auto result = std::distance(begin(directions), find_result);
		AdventCheck(utils::range_contains_exc(result, 0, 8));
		return static_cast<std::size_t>(result);
	}

	DirMask get_dirmask(std::size_t idx)
	{
		return DirMask{ 1 } << idx;
	}

	DirMask get_dirmask(Coords dir)
	{
		const auto idx = get_dir_idx(dir);
		return get_dirmask(idx);
	}

	DirMask get_neighbour_mask(const AreaMap& map, const Coords& coord)
	{
		DirMask result{ 0 };
		const auto neighbours = coord.neighbours_plus_diag();
		for (const Coords& neighbour : neighbours)
		{
			if (!is_free(map, neighbour))
			{
				const Coords dir = neighbour - coord;
				const DirMask mask = get_dirmask(dir);
				result = result | mask;
			}
		}
		return result;
	}

	DirMask get_spaces_to_check(Dir direction)
	{
		std::array<Coords, 3> dirs_to_check;

		const bool h = utils::is_horizontal(direction);
		dirs_to_check[0] = Coords::dir(direction);
		dirs_to_check[1] = dirs_to_check[0] + (h ? Coords::up() : Coords::left());
		dirs_to_check[2] = dirs_to_check[0] + (h ? Coords::down() : Coords::right());

		DirMask result{ 0 };
		for (const Coords& c : dirs_to_check)
		{
			const DirMask new_mask = get_dirmask(c);
			result = result | new_mask;
		}
		return result;
	}

	AreaMap parse_area(std::istream& input)
	{
		AreaMap result;
		int y = 0;
		for(int y = 0; !input.eof(); ++y)
		{
			std::string line;
			std::getline(input, line);
			const int line_len = static_cast<int>(std::ssize(line));
			for (int x : utils::int_range{line_len})
			{
				switch (line[x])
				{
				case ELF_SYMBOL:
					result.push_back(Coords{ x,-y }); // Invert y so +ve y is NORTH.
					break;
				default:
					break;
				}
			}
		}
		return result;
	}

	std::pair<Coords, Coords> get_bounds(const AreaMap& map)
	{
		Coords minima{ std::numeric_limits<int>::max() };
		Coords maxima{ std::numeric_limits<int>::min() };

		for (const Coords& coords : map)
		{
			minima.x = std::min(minima.x, coords.x);
			minima.y = std::min(minima.y, coords.y);
			maxima.x = std::max(maxima.x, coords.x);
			maxima.y = std::max(maxima.y, coords.y);
		}
		return std::make_pair(minima, maxima + Coords{ 1 });
	}

	int get_area(const Coords& minima, const Coords& maxima)
	{
		const Coords diff = maxima - minima;
		return diff.x * diff.y;
	}

	int get_area(const AreaMap& map)
	{
		const auto [minima, maxima] = get_bounds(map);
		return get_area(minima, maxima);
	}

	std::string dump_map(const AreaMap& map)
	{
		std::string result;
		const auto [minima, maxima] = get_bounds(map);
		result.reserve(get_area(minima, maxima) + (maxima.y - minima.y));

		for (auto y : utils::int_range{ maxima.y - 1, minima.y - 1, -1 })
		{
			for (auto x : utils::int_range{ minima.x, maxima.x })
			{
				const bool has_elf = !is_free(map, Coords{ x,y });
				result.push_back(has_elf ? ELF_SYMBOL : EMPTY_SYMBOL);
			}
			result.push_back('\n');
		}
		return result;
	}

	struct ScratchArea
	{
	private:
		std::vector<Coords> prop_coords;
		AreaMap wip_map;
		AreaMap final_map;
	public:
		void reset(std::size_t capacity)
		{
			auto do_reset = [capacity](auto& thing)
			{
				thing.clear();
				thing.reserve(capacity);
			};
			do_reset(prop_coords);
			do_reset(wip_map);
			do_reset(final_map);
		}

		void add_prop(const Coords& prop)
		{
			prop_coords.push_back(prop);
			wip_map.push_back(prop);
		}

		// Returns true if anything actually moved
		bool create_final_map(AreaMap& original_map)
		{
			bool move_made = false;
			AdventCheck(original_map.size() == prop_coords.size());
			AdventCheck(original_map.size() == wip_map.size());
			for (std::size_t i : utils::int_range{ original_map.size() })
			{
				const auto num_targets = wip_map.count(prop_coords[i]);
				AdventCheck(num_targets > decltype(num_targets){0});
				const bool can_move = (num_targets == 1) && prop_coords[i] != (original_map[i]);
				const Coords result = can_move ? prop_coords[i] : original_map[i];
				final_map.push_back(result);
				move_made = move_made || can_move;
			}
			std::swap(original_map, final_map);
			return move_made;
		}
	};

	// Returns true if any elves actually moved.
	bool move_elves(AreaMap& map, const std::array<utils::direction, 4>& search_pattern, ScratchArea& scratch)
	{
		scratch.reset(map.size());

		auto get_proposed_move = [&map, &search_pattern](const Coords& loc)
		{
			const DirMask neighbours = get_neighbour_mask(map, loc);
			if (neighbours == DirMask{ 0 }) return loc; // No neighbours so don't move.
			for (Dir d : search_pattern)
			{
				const DirMask spaces = get_spaces_to_check(d);
				const DirMask overlap = neighbours & spaces;
				if (overlap == DirMask{ 0 }) return loc + Coords::dir(d);
			}
			return loc;

		};

		auto add_prop = [&get_proposed_move, &scratch](const Coords& from)
		{
			const Coords prop = get_proposed_move(from);
			scratch.add_prop(prop);
		};

		std::ranges::for_each(map, add_prop);
		const bool moved_any_elves = scratch.create_final_map(map);
		return moved_any_elves;
	}

	struct SimulateResult
	{
		AreaMap final_map;
		int num_moves = 0;
	};

	SimulateResult simulate(std::istream& input, int max_moves = std::numeric_limits<int>::max())
	{
		SimulateResult result;
		result.final_map = parse_area(input);
		result.num_moves = max_moves;

		ScratchArea scratch_area;
		std::array<Dir, 4> search_pattern{ Dir::up, Dir::down, Dir::left, Dir::right };
		for (auto i : utils::int_range{ max_moves })
		{
			log << "\nStart of move " << i + 1 << ":\n" << dump_map(result.final_map);
			const bool moved = move_elves(result.final_map, search_pattern, scratch_area);
			if (!moved)
			{
				result.num_moves = i + 1;
				break;
			}
			std::ranges::rotate(search_pattern, begin(search_pattern) + 1);
		}
		log << "\nFinal map:\n" << dump_map(result.final_map) << '\n';
		return result;
	}

	int solve_p1_generic(std::istream& input, int max_moves)
	{
		const SimulateResult result = simulate(input, max_moves);
		const int area = get_area(result.final_map);
		const int num_elves = static_cast<int>(result.final_map.size());
		return area - num_elves;
	}

	int solve_p1(std::istream& input)
	{
		return solve_p1_generic(input,10);
	}
}

namespace
{
	int solve_p2(std::istream& input)
	{
		const SimulateResult result = simulate(input);
		return result.num_moves;
	}
}

namespace
{

	auto testcase_a()
	{
		return std::istringstream{
R"""(.....
..##.
..#..
.....
..##.
.....)"""
		};
	}

	auto testcase_b()
	{
		return std::istringstream{
R"""(....#..
..###.#
#...#.#
.#...##
#.###..
##.#.##
.#..#..)"""
		};
	}
}

ResultType day_twentythree_p1_a()
{
	auto input = testcase_a();
	return solve_p1(input);
}

ResultType day_twentythree_p1_b()
{
	auto input = testcase_b();
	return solve_p1(input);
}

ResultType day_twentythree_p2_a()
{
	auto input = testcase_a();
	return solve_p2(input);
}

ResultType day_twentythree_p2_b()
{
	auto input = testcase_b();
	return solve_p2(input);
}

ResultType advent_twentythree_p1()
{
	auto input = advent::open_puzzle_input(23);
	return solve_p1(input);
}

ResultType advent_twentythree_p2()
{
	auto input = advent::open_puzzle_input(23);
	return solve_p2(input);
}

#undef DAY23DBG
#undef ENABLE_DAY23DBG