#include "advent24.h"
#include "../advent/advent_utils.h"

#define ENABLE_DAY24DBG 1
#ifdef NDEBUG
#define DAY24DBG 0
#else
#define DAY24DBG ENABLE_DAY24DBG
#endif

#if DAY24DBG
	#include <iostream>
#endif

namespace
{
#if DAY24DBG
	std::ostream & log = std::cout;
#else
	struct {	template <typename T> auto& operator<<(const T&) const noexcept { return *this; } } log;
#endif
}

#include "coords.h"
#include "small_vector.h"
#include "range_contains.h"
#include "modular_int.h"
#include "istream_line_iterator.h"
#include "comparisons.h"
#include "swap_remove.h"
#include <memory>
#include "int_range.h"

namespace
{
	using utils::direction;
	using utils::coords;

	void validate(char c)
	{
		constexpr std::string_view allowed_chars{ ".#SF^v<>" };
		AdventCheck(allowed_chars.contains(c));
	}

	enum class BlockType : char
	{
		empty = '.',
		wall = '#',
		start = 'S',
		end = 'F',
		blizzard_up = '^',
		blizzard_down = 'v',
		blizzard_left = '<',
		blizzard_right = '>'
	};

	BlockType to_block(char c)
	{
		validate(c);
		return static_cast<BlockType>(c);
	}

	char to_char(BlockType b)
	{
		return static_cast<char>(b);
	}

	std::ostream& operator<<(std::ostream& os, BlockType b)
	{
		os << to_char(b);
		return os;
	}

	using DownBlizzardLocations = utils::small_vector<int, 13>;
	using LeftBlizzardLocations = utils::small_vector<int, 31>;
	using RightBlizzardLocations = utils::small_vector<int, 37>;
	using UpBlizzardLocations = utils::small_vector<int, 16>;

	template <typename T>
	using HorizontalBlizzards = utils::small_vector<T, 102>;
	using LeftBlizzards = HorizontalBlizzards<LeftBlizzardLocations>;
	using RightBlizzards = HorizontalBlizzards<RightBlizzardLocations>;

	template <typename T>
	using VerticalBlizzards = utils::small_vector<T, 37>;
	using DownBlizzards = VerticalBlizzards<DownBlizzardLocations>;
	using UpBlizzards = VerticalBlizzards<UpBlizzardLocations>;

	template <std::size_t Major, std::size_t Minor>
	using GenericBlizzardHolder = utils::small_vector<utils::small_vector<int, Minor>, Major>;

	// Memoise blizzard locations
	enum class MemoState : uint8_t
	{
		Unmemoized = 0,
		NoBlizzard = 1,
		HasBlizzard = 2
	};

	// Holds 4 memos
	class MemoFragment
	{
		MemoState a : 2;
		MemoState b : 2;
		MemoState c : 2;
		MemoState d : 2;
	public:
		static constexpr std::size_t NumMemos = 4;
		MemoFragment()
		{
			static_assert(sizeof(*this) == 1);
			std::memset(this, 0, sizeof(*this));
		}
		MemoState get(std::size_t idx) const
		{
			switch (idx)
			{
			case 0:
				return a;
			case 1:
				return b;
			case 2:
				return c;
			case 3:
				return d;
			default:
				break;
			}
			AdventUnreachable();
			return MemoState::Unmemoized;
		}
		void set(std::size_t idx, MemoState new_state)
		{
			switch (idx)
			{
			case 0:
				a = new_state;
				return;
			case 1:
				b = new_state;
				return;
			case 2:
				c = new_state;
				return;
			case 3:
				d = new_state;
				return;
			default:
				break;
			}
			AdventUnreachable();
		}
	};

	// Holds as many memos as we need.
	class BlizzardMemos
	{
		coords minima;
		int64_t page_size = 0; // One page per minute
		int64_t x_width = 0;
		std::vector<MemoFragment> memo_data;
		static std::pair<std::size_t, std::size_t> get_data_frag_indices(std::size_t idx)
		{
			const std::size_t data_idx = idx / MemoFragment::NumMemos;
			const std::size_t frag_idx = idx % MemoFragment::NumMemos;
			return { data_idx, frag_idx };
		}
		MemoState get_memo(std::size_t idx) const
		{
			const auto [data_idx, frag_idx] = get_data_frag_indices(idx);
			return data_idx < memo_data.size() ? memo_data[data_idx].get(frag_idx) : MemoState::Unmemoized;
		}
		std::size_t to_idx(const coords& pos, int64_t minute) const
		{
			AdventCheck(page_size != 0);
			AdventCheck(x_width != 0);
			const coords offset = pos - minima;
			AdventCheck(utils::range_contains_exc(offset.x, 0, x_width));
			AdventCheck(utils::range_contains_exc(offset.y, 0, page_size / x_width));
			const int64_t page_pos = int64_t{ offset.y } * x_width + int64_t{ offset.x };
			const int64_t page_base = minute * page_size;
			const int64_t result = page_pos + page_base;
			AdventCheck(result > 0);
			AdventCheck(static_cast<uint64_t>(result) <= std::numeric_limits<std::size_t>::max());
			return static_cast<std::size_t>(result);
		}

		void set_memo(std::size_t idx, bool has_blizzard)
		{
			const auto [data_idx, frag_idx] = get_data_frag_indices(idx);
			if (data_idx >= memo_data.size())
			{
				memo_data.resize(data_idx + 1);
			}
			memo_data[data_idx].set(frag_idx, has_blizzard ? MemoState::HasBlizzard : MemoState::NoBlizzard);
		}
	public:
		void initialize(const coords& bottom_left, const coords& top_right)
		{
			minima = bottom_left + coords{ 1 };
			const coords sizes = top_right - minima;
			x_width = sizes.x;
			page_size = sizes.x * sizes.y;
		}

		MemoState get_memo(const coords& pos, int minute) const
		{
			const std::size_t memo_idx = to_idx(pos, minute);
			return get_memo(memo_idx);
		}
		void set_memo(const coords& pos, int minute, bool has_blizzard)
		{
			const std::size_t memo_idx = to_idx(pos, minute);
			set_memo(memo_idx, has_blizzard);
		}
	};

	// All locations are at minute 0.
	struct Map
	{
		coords start_loc;
		coords finish_loc;
		coords bottom_left{ std::numeric_limits<int>::max() };
		coords top_right{ std::numeric_limits<int>::min() };
		DownBlizzards down_blizzards;
		LeftBlizzards left_blizzards;
		RightBlizzards right_blizzards;
		UpBlizzards up_blizzards;
		mutable BlizzardMemos blizard_memoization;

		Map() = default;

		bool can_step_here(const coords& pos, int minute) const
		{
			return is_traversible(pos) && !has_blizzard(pos, minute);
		}

		bool has_blizzard(const coords& pos, int minute) const
		{
			if (pos == start_loc) return false;
			if (pos == finish_loc) return false;
			const MemoState memo_result = blizard_memoization.get_memo(pos, minute);
			switch (memo_result)
			{
			case MemoState::Unmemoized:
			{
				const bool result = has_blizzard_impl(pos, minute);
				blizard_memoization.set_memo(pos, minute, result);
				return result;
			}
			case MemoState::NoBlizzard:
				return false;
			case MemoState::HasBlizzard:
				return true;
			default:
				break;
			}
			AdventUnreachable();
			return false;
		}

		bool is_traversible(const coords& c) const
		{
			using utils::range_contains_exc;
			if (c == start_loc) return true;
			if (c == finish_loc) return true;
			if (!range_contains_exc(c.x, bottom_left.x + 1, top_right.x)) return false;
			return range_contains_exc(c.y, bottom_left.y + 1, top_right.y);
		}

		template <direction dir, std::size_t Major, std::size_t Minor>
		bool has_blizzard_impl(const coords& pos, const GenericBlizzardHolder<Major,Minor>& blizzard_holder, int minutes) const
		{
			auto check_addr = [](const auto& left, const auto& right)
			{
				const void* left_ptr = reinterpret_cast<const void*>(&left);
				const void* right_ptr = reinterpret_cast<const void*>(&right);
				AdventCheck(left_ptr == right_ptr);
			};
			switch (dir)
			{
			case direction::down:
				check_addr(blizzard_holder, down_blizzards);
				break;
			case direction::left:
				check_addr(blizzard_holder, left_blizzards);
				break;
			case direction::right:
				check_addr(blizzard_holder, right_blizzards);
				break;
			case direction::up:
				check_addr(blizzard_holder, up_blizzards);
				break;
			default:
				AdventUnreachable();
				break;
			}

			constexpr bool h = utils::is_horizontal(dir);
			const coords blizzard_delta = coords::dir(dir);
			const int min_position = (h ? bottom_left.x : bottom_left.y) + 1;
			const int max_position = (h ? top_right.x : top_right.y);
			const int static_coord = h ? pos.y : pos.x;
			const int coord_delta = minutes * (h ? blizzard_delta.x : blizzard_delta.y);
			const int target_coord = h ? pos.x : pos.y;
			const auto& blizzard_line = blizzard_holder[static_coord];

			auto hits_loc = [coord_delta, min_position, max_position, target_coord](int moving_coord)
			{
				const int target_location = moving_coord + coord_delta;
				const int final_location = utils::get_unwound(target_location, min_position, max_position);
				return final_location == target_coord;
			};

			return std::ranges::any_of(blizzard_line, hits_loc);
		}

		bool has_blizzard_impl(const coords& location, int minute) const
		{
			return
				has_blizzard_impl<direction::down>(location, down_blizzards, minute) ||
				has_blizzard_impl<direction::left>(location, left_blizzards, minute) ||
				has_blizzard_impl<direction::right>(location, right_blizzards, minute) ||
				has_blizzard_impl<direction::up>(location, up_blizzards, minute);
		}

		char get_token(const coords& pos, int minute) const
		{
			if (!is_traversible(pos)) return to_char(BlockType::wall);
			BlockType blizzard_type = BlockType::empty;
			int blizzards_found = 0;
			if (has_blizzard_impl<direction::up>(pos, up_blizzards, minute))
			{
				blizzard_type = BlockType::blizzard_up;
				++blizzards_found;
			}
			if (has_blizzard_impl<direction::right>(pos, right_blizzards, minute))
			{
				blizzard_type = BlockType::blizzard_right;
				++blizzards_found;
			}
			if (has_blizzard_impl<direction::down>(pos, down_blizzards, minute))
			{
				blizzard_type = BlockType::blizzard_down;
				++blizzards_found;
			}
			if (has_blizzard_impl<direction::left>(pos, left_blizzards, minute))
			{
				blizzard_type = BlockType::blizzard_left;
				++blizzards_found;
			}
			if (blizzards_found == 1)
			{
				return to_char(blizzard_type);
			}
			if (blizzards_found > 1)
			{
				return '0' + static_cast<char>(blizzards_found);
			}
			if (pos == start_loc) return to_char(BlockType::start);
			if (pos == finish_loc) return to_char(BlockType::end);
			return to_char(BlockType::empty);
		}

		std::string print(int minute) const
		{
			std::ostringstream result;
			result << '\n';
			for (int y = top_right.y; y >= bottom_left.y; --y)
			{
				for (int x = bottom_left.x; x <= top_right.x; ++x)
				{
					const coords loc{ x,y };
					result << get_token(loc, minute);
				}
				result << '\n';
			}
			return result.str();
		}
	};

	std::unique_ptr<Map> parse_map(std::istream& input)
	{
		using ILI = utils::istream_line_iterator;
		std::vector<std::string> input_lines;
		std::transform(ILI{ input }, ILI{}, std::back_inserter(input_lines), [](std::string_view line) {return std::string{ line };  });

		AdventCheck(!input_lines.empty());
		auto is_line_correct = [target = input_lines.front().size()](std::string_view line)
		{
			const char w = to_char(BlockType::wall);
			return line.size() == target
				&& line.front() == w
				&& line.back() == w;
		};
		AdventCheck(std::ranges::all_of(input_lines, is_line_correct));
		const int num_lines = static_cast<int>(input_lines.size());
		const int line_width = static_cast<int>(input_lines.front().size());

		std::unique_ptr<Map> result = std::make_unique<Map>();
		AdventCheck(result.get() != nullptr);
		result->bottom_left = coords{ 0,0 };
		result->top_right = coords{ line_width, num_lines } - coords{ 1 , 1 };

		auto get_hole_in_wall = [](std::string_view line)
		{
			auto is_valid_char = [](char c)
			{
				const BlockType b = to_block(c);
				switch (b)
				{
				case BlockType::wall:
				case BlockType::empty:
					return true;
				default:
					break;
				}
				return false;
			};
			AdventCheck(std::ranges::all_of(line, is_valid_char));
			
			auto get_num_openings = [line]()
			{
				const auto result = std::ranges::count(line, to_char(BlockType::empty));
				return static_cast<int>(result);
			};
			AdventCheck(get_num_openings() == 1);

			const std::size_t location = line.find(to_char(BlockType::empty));
			return static_cast<int>(location);
		};

		result->start_loc.y = num_lines-1;
		result->start_loc.x = get_hole_in_wall(input_lines.front());
		result->finish_loc.y = 0;
		result->finish_loc.x = get_hole_in_wall(input_lines.back());

		log << "\nFound start and finish: " << result->start_loc << " --> " << result->finish_loc;

		result->up_blizzards.resize(line_width);
		result->down_blizzards.resize(line_width);
		result->left_blizzards.resize(num_lines);
		result->right_blizzards.resize(num_lines);

		for (std::size_t i : utils::int_range<std::size_t>{ 1, input_lines.size() - 1 })
		{
			const int y = num_lines - 1 - static_cast<int>(i);
			const std::string_view line = input_lines[i];
			for (int x : utils::int_range<int>{ 1, line_width - 1 })
			{
				const coords location{ x,y };
				const char c = line[x];
				const BlockType b = to_block(c);
				switch (b)
				{
				case BlockType::empty:
					break;
				case BlockType::blizzard_up:
					//log << "\nAdding up blizzard at " << location << "[Num coords: " << result->up_blizzards.size() << ']';
					AdventCheck(result->up_blizzards.size() > static_cast<std::size_t>(x));
					result->up_blizzards[x].push_back(y);
					break;
				case BlockType::blizzard_down:
					//log << "\nAdding down blizzard at " << location << "[Num coords: " << result->down_blizzards.size() << ']';
					AdventCheck(result->down_blizzards.size() > static_cast<std::size_t>(x));
					result->down_blizzards[x].push_back(y);
					break;
				case BlockType::blizzard_left:
					//log << "\nAdding left blizzard at " << location << "[Num coords: " << result->left_blizzards.size() << ']';
					AdventCheck(result->left_blizzards.size() > static_cast<std::size_t>(y));
					result->left_blizzards[y].push_back(x);
					break;
				case BlockType::blizzard_right:
					//log << "\nAdding right blizzard at " << location << "[Num coords: " << result->right_blizzards.size() << ']';
					AdventCheck(result->right_blizzards.size() > static_cast<std::size_t>(y));
					result->right_blizzards[y].push_back(x);
					break;
				default:
					AdventUnreachable();
					break;
				}
			}
		}
		result->blizard_memoization.initialize(result->bottom_left, result->top_right);
#if DAY24DBG
		auto get_most = [](const auto& container)
		{
			auto container_size = [](const decltype(container.front())& ic)
			{
				return std::ssize(ic);
			};

			const auto& biggest = utils::ranges::max_transform(container, container_size);
			return std::ssize(biggest);
		};

		log << "\nMax up  blizzards: " << get_most(result->up_blizzards);
		log << "\nMax down blizzards: " << get_most(result->down_blizzards);
		log << "\nMax left blizzards: " << get_most(result->left_blizzards);
		log << "\nMax right blizzards: " << get_most(result->right_blizzards) << '\n';
#endif
		return result;
	}

	int find_route_length(const Map& map, int start_minute, bool is_reversed)
	{
		struct Node
		{
			coords location;
			int time;
			int heuristic;
			int64_t previous_idx;
		};

		std::vector<Node> searched_nodes;
		const coords& start_loc = is_reversed ? map.finish_loc : map.start_loc;
		const coords& finish_loc = is_reversed ? map.start_loc : map.finish_loc;

		auto make_node = [&finish_loc, &searched_nodes](const coords& loc, int t)
		{
			return Node{ loc, t, loc.manhatten_distance(finish_loc) + t, std::ssize(searched_nodes) - 1 };
		};
		std::vector<Node> unsearched_nodes{ make_node(start_loc, start_minute) };

		auto try_add_node = [&map, &unsearched_nodes, &make_node](const coords& loc, int t)
		{
			if (map.can_step_here(loc, t))
			{
				unsearched_nodes.push_back(make_node(loc, t));
			}
		};

		while (!unsearched_nodes.empty())
		{
			const auto get_heuristic = [](const Node& n) { return n.heuristic; };
			const auto node_to_check_it = utils::ranges::min_element_transform(unsearched_nodes, get_heuristic);
			AdventCheck(node_to_check_it != end(unsearched_nodes));
			const Node node_to_check = *node_to_check_it;

			if (node_to_check.location == finish_loc)
			{
				return node_to_check.time;
			}

			utils::swap_remove(unsearched_nodes, node_to_check_it);

			const auto nodes_match = [&node_to_check](const Node& n)
			{
				return n.location == node_to_check.location && n.time == node_to_check.time;
			};
			const auto previously_checked = std::ranges::find_if(searched_nodes, nodes_match);

			if (previously_checked != end(searched_nodes)) continue;

			searched_nodes.push_back(node_to_check);

			const auto neighbours = node_to_check.location.neighbours();
			const int new_time = node_to_check.time + 1;

			// Push a wait.
			for (const coords& n : neighbours)
			{
				try_add_node(n, new_time);
			}
			try_add_node(node_to_check.location, new_time);
		}
		AdventUnreachable();
		return -1;
	}

	int solve_generic(std::istream& input, int times_across)
	{
		const std::unique_ptr<Map> map = parse_map(input);
		if (map.get() == nullptr)
		{
			return -1;
		}

		int total_time = 0;
		for (int i : utils::int_range(times_across))
		{
			total_time = find_route_length(*map, total_time, i % 2);
		}
		return total_time;
	}

	int solve_p1(std::istream& input)
	{
		return solve_generic(input, 1);
	}
}

namespace
{
	int solve_p2(std::istream& input)
	{
		return solve_generic(input, 3);
	}
}

namespace
{
	std::istringstream testcase_a()
	{
		return std::istringstream
		{
			"#.#####\n"
			"#.....#\n"
			"#>....#\n"
			"#.....#\n"
			"#...v.#\n"
			"#.....#\n"
			"#####.#"
		};
	}

	std::istringstream testcase_b()
	{
		return std::istringstream
		{
			"#.######\n"
			"#>>.<^<#\n"
			"#.<..<<#\n"
			"#>v.><>#\n"
			"#<^v^^>#\n"
			"######.#"
		};
	}
}

ResultType advent_twentyfour_p1()
{
	auto input = advent::open_puzzle_input(24);
	return solve_p1(input);
}

ResultType advent_twentyfour_p2()
{
	auto input = advent::open_puzzle_input(24);
	return solve_p2(input);
}

ResultType day_twentyfour_p1_a()
{
	auto input = testcase_a();
	return solve_p1(input);
}

ResultType day_twentyfour_p1_b()
{
	auto input = testcase_b();
	return solve_p1(input);
}

ResultType day_twentyfour_p2_a()
{
	auto input = testcase_a();
	return solve_p2(input);
}

ResultType day_twentyfour_p2_b()
{
	auto input = testcase_b();
	return solve_p2(input);
}

#undef DAY24DBG
#undef ENABLE_DAY24DBG