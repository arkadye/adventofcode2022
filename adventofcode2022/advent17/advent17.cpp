#include "advent17.h"
#include "../advent/advent_utils.h"

#define ENABLE_DAY17DBG 1
#ifdef NDEBUG
#define DAY17DBG 0
#else
#define DAY17DBG ENABLE_DAY17DBG
#endif

#if DAY17DBG
	#include <iostream>
#endif

namespace
{
#if DAY17DBG
	std::ostream & log = std::cout;
#else
	struct {	template <typename T> auto& operator<<(const T&) const noexcept { return *this; } } log;
#endif
}

#include "range_contains.h"
#include "string_line_iterator.h"
#include "int_range.h"
#include "coords.h"
#include "small_vector.h"
#include "comparisons.h"
#include "sorted_vector.h"

namespace
{
	using coords = utils::basic_coords<int64_t>;
	constexpr int64_t SPRITE_MAX = 4;
	constexpr std::size_t NUM_ROCK_TYPES = 5;
	constexpr int64_t STARTING_HEIGHT = 4;
	constexpr int64_t STARTING_FROM_LEFT = 3;

	enum class Wind : char
	{
		left = '<',
		right = '>'
	};

	Wind to_wind(char c)
	{
		AdventCheck(std::string_view{ "<>" }.contains(c));
		return static_cast<Wind>(c);
	}

	char to_3char (Wind w)
	{
		return static_cast<char>(w);
	}

	enum class Tile : char
	{
		block = '#',
		empty = ' ',
		dynamic_block = '@'
	};

	Tile to_tile(char c)
	{
		if (c == '.') c = ' ';
		AdventCheck(std::string_view{ " #" }.contains(c));
		return static_cast<Tile>(c);
	}

	char to_char(Tile t)
	{
		return static_cast<char>(t);
	}

	struct Sprite
	{
	private:
		friend class SpriteSheet;
		friend std::ostream& operator<<(std::ostream& os, Sprite s);
		friend bool do_sprites_overlap(Sprite left, Sprite right);
		uint16_t m_data;
		uint16_t get_location_mask(const coords& offset) const
		{
			AdventCheck(utils::range_contains_exc(offset.x,0,SPRITE_MAX));
			AdventCheck(utils::range_contains_exc(offset.y,0,SPRITE_MAX));
			const int64_t mask_offset = 4 * offset.y + offset.x;
			return uint16_t{ 1 } << mask_offset;
		}
	public:
		Sprite() : m_data{ 0 } {}
		explicit Sprite(std::string_view init) { from_string(init); }
		Tile get_tile(const coords& offset) const
		{
			if (!utils::range_contains_exc(offset.x, 0, SPRITE_MAX)) return Tile::empty;
			if (!utils::range_contains_exc(offset.y, 0, SPRITE_MAX)) return Tile::empty;
			const uint16_t mask = get_location_mask(offset);
			const uint16_t result = (mask & m_data);
			return (result != uint16_t{ 0 }) ? Tile::block : Tile::empty;
		}
		void set_tile(const coords& offset, Tile t)
		{
			const uint16_t mask = get_location_mask(offset);
			switch (t)
			{
			case Tile::block:
				m_data = m_data | mask;
				break;
			case Tile::empty:
				m_data = m_data & ~mask;
				break;
			default:
				AdventUnreachable();
				break;
			}
		}
		void from_string(std::string_view sv)
		{
			const int num_lines = static_cast<int>(std::ranges::count(sv, '\n')) + 1;
			m_data = 0;
			int y_idx = 0;
			for (std::string_view line : utils::string_line_range{ sv })
			{
				AdventCheck(y_idx < num_lines);
				const int y_coord = num_lines - 1 - y_idx;
				int x_coord = 0;
				for (char c : line)
				{
					AdventCheck(x_coord < SPRITE_MAX);
					const Tile t = to_tile(c);
					const coords coords{ x_coord,y_coord };
					set_tile(coords, t);
					++x_coord;
				}
				++y_idx;
			}
		}

		Sprite get_fragment(const coords& offset) const
		{
			Sprite sprite;
			if (std::abs(offset.x) >= SPRITE_MAX) return sprite;
			if (std::abs(offset.y) >= SPRITE_MAX) return sprite;
			for (int64_t my_y : utils::int_range{ SPRITE_MAX })
			{
				const int64_t target_y = my_y + offset.y;
				const bool y_in_range = utils::range_contains_exc(target_y, 0, SPRITE_MAX);
				if (!y_in_range) continue;

				for (int64_t my_x : utils::int_range{ SPRITE_MAX })
				{
					const int64_t target_x = my_x + offset.x;
					const bool x_in_range = utils::range_contains_exc(target_x, 0, SPRITE_MAX);
					if (!x_in_range) continue;

					const Tile tile = get_tile(coords{ my_x, my_y });
					sprite.set_tile(coords{ target_x, target_y }, tile);
				}
			}
			//log << (*this) << "\n   fragment at " << offset << ": " << sprite;
			return sprite;
		}

		Sprite& operator+=(Sprite other)
		{
			m_data = m_data | other.m_data;
			return *this;
		}

		int64_t get_height() const
		{
			const uint16_t base_mask = 0b1111;
			for (int64_t i : utils::int_range{ SPRITE_MAX })
			{
				const uint16_t line_mask = base_mask << (i * SPRITE_MAX);
				const uint16_t masked = line_mask & m_data;
				if (masked == 0) return i;
			}
			return SPRITE_MAX;
		}
	};

	std::ostream& operator<<(std::ostream& os, Sprite s)
	{
		const utils::int_range range{ SPRITE_MAX };
		os << "\n+----+";
		for (auto it = rbegin(range); it != rend(range); ++it)
		{
			const int64_t row = *it;
			os << "\n|";
			for (int64_t col : range)
			{
				const coords loc{ col,row };
				const Tile tile = s.get_tile(loc);
				const char c = to_char(tile);
				os << c;
			}
			os << '|';
		}
		os << "\n+----+";
		return os;
	}

	class SpriteSheet
	{
		utils::small_vector<uint64_t, 4> m_data;
	public:
		Sprite get(std::size_t idx) const
		{
			const std::size_t outer_idx = idx / 4;
			
			if (outer_idx >= m_data.size()) [[unlikely]] return Sprite{};

			const uint64_t result_tile = m_data[outer_idx];
			const std::size_t inner_idx = idx % 4;

			const std::size_t shift_offset = inner_idx * CHAR_BIT;
			constexpr uint64_t base_mask = 0xFF;

			const uint64_t shifted_tile = result_tile >> shift_offset;
			const uint64_t result_data = shifted_tile & base_mask;

			Sprite result;
			result.m_data = static_cast<uint16_t>(result_data);
			return result;
		}

		void set(std::size_t idx, Sprite sprite)
		{
			const std::size_t outer_idx = idx / 4;
			if (outer_idx >= m_data.size()) [[unlikely]]
			{
				m_data.resize(outer_idx + 1);
			}

			const uint64_t tile_data = m_data[outer_idx];
			const std::size_t inner_idx = idx % 4;
			const std::size_t shift_offset = (inner_idx * CHAR_BIT);

			constexpr uint64_t base_mask = 0xFF;
			const uint64_t and_mask = base_mask << shift_offset;
			const uint64_t zeroed_data = tile_data & ~and_mask;

			const uint64_t shifted_data = uint64_t{ sprite.m_data } << shift_offset;
			const uint64_t new_data = shifted_data | zeroed_data;
			m_data[outer_idx] = new_data;
		}

		// Top left is (0,0).
		void set(Tile tile, const coords& point, int64_t width_in_sprites)
		{
			AdventCheck(point.y <= 0);
			const int64_t h_sprite = point.x / SPRITE_MAX;
			AdventCheck(h_sprite < width_in_sprites);

			const int64_t v_sprite = (-point.y) / SPRITE_MAX;
			const std::size_t sprite_idx = static_cast<std::size_t>(width_in_sprites * v_sprite + h_sprite);

			const int64_t sprite_x = point.x % SPRITE_MAX;
			const int64_t sprite_y = SPRITE_MAX - ((-point.y) % SPRITE_MAX) - 1;

			const coords sprite_coords{ sprite_x,sprite_y };

			Sprite point_sprite;
			point_sprite.set_tile(sprite_coords, tile);

			set(sprite_idx, point_sprite);
		}

		constexpr std::strong_ordering operator<=>(const SpriteSheet& other) const
		{
			const std::size_t my_size = m_data.size();
			const std::size_t other_size = other.m_data.size();
			if (my_size < other_size)
			{
				const std::strong_ordering result = (other <=> *this);
				if (result == std::strong_ordering::greater) return std::strong_ordering::less;
				if (result == std::strong_ordering::less) return std::strong_ordering::greater;
				return result;
			}

			std::size_t i = 0u;

			while (i < other_size)
			{
				const std::strong_ordering check = m_data[i] <=> other.m_data[i];
				if (check != std::weak_ordering::equivalent)
				{
					return check;
				}
				++i;
			}

			while (i < my_size)
			{
				if (m_data[i] != 0)
				{
					return std::strong_ordering::greater;
				}
				++i;
			}

			const bool sizes_equal = my_size == other_size;
			return sizes_equal ? std::strong_ordering::equal : std::strong_ordering::equivalent;
		}
	};

	enum class SpriteType
	{
		horizontal_line,
		cross,
		right_angle,
		vertical_line,
		square,
		left_wall,
		right_wall,
		bottom,
		empty
	};

	SpriteType get_block_from_idx(std::size_t idx)
	{
		constexpr std::array<SpriteType, NUM_ROCK_TYPES> blocks{
			SpriteType::horizontal_line,
			SpriteType::cross,
			SpriteType::right_angle,
			SpriteType::vertical_line,
			SpriteType::square
		};

		return blocks[idx % NUM_ROCK_TYPES];
	}

	Sprite get_default_sprite(SpriteType type)
	{
		switch (type)
		{
		case SpriteType::cross:
			return Sprite{
				".#.\n"
				"###\n"
				".#."
			};
		case SpriteType::right_angle:
			return Sprite{
				"..#\n"
				"..#\n"
				"###"
			};
		case SpriteType::right_wall:
		case SpriteType::left_wall:
		case SpriteType::vertical_line:
			return Sprite{ "#\n#\n#\n#" };
		case SpriteType::square:
			return Sprite{ "##\n##" };
		case SpriteType::horizontal_line:
		case SpriteType::bottom:
			return Sprite{ "####" };
		case SpriteType::empty:
			return Sprite{};
		default:
			AdventUnreachable();
			break;
		}
		return Sprite{};
	}

	Sprite operator+(Sprite left, Sprite right)
	{
		left += right;
		return left;
	}

	bool do_sprites_overlap(Sprite left, Sprite right)
	{
		const uint16_t result = left.m_data & right.m_data;
		//log << "\nOverlap test:" << left << "\n   v" << right << "\nResult: " << result;
		return result != 0;
	}

	bool do_sprites_overlap(Sprite left, Sprite right, const coords& right_offset)
	{
		const Sprite new_left = left.get_fragment(right_offset);
		return do_sprites_overlap(new_left, right);
	}

	class Block
	{
		friend bool do_blocks_overlap(const Block& left, const Block& right);
		coords location;
		Sprite sprite;
	public:
		void set_sprite(Sprite s)
		{
			sprite = s;
		}
		void set_sprite(SpriteType type)
		{
			const Sprite s = get_default_sprite(type);
			set_sprite(s);
		}
		void set_sprite(std::size_t rock_idx)
		{
			const SpriteType st = get_block_from_idx(rock_idx);
			set_sprite(st);
		}
		void set_location(const coords& new_location)
		{
			location = new_location;
		}
		coords get_location() const
		{
			return location;
		}
		int64_t get_height() const
		{
			return sprite.get_height();
		}
		int64_t get_top_y() const
		{
			const int64_t height = get_height();
			const int64_t result = height + location.y - 1;
			return result;
		}
		Block& operator+=(const Block& other)
		{
			const coords offset = other.location - location;
			if (std::abs(offset.x) < SPRITE_MAX)
			{
				if (std::abs(offset.y) < SPRITE_MAX)
				{
					const Sprite masked_other = other.sprite.get_fragment(offset);
					sprite += masked_other;
				}
			}
			return *this;
		}
		Sprite get_sprite() const
		{
			return sprite;
		}
		Tile get_tile(coords world_location) const
		{
			const coords local_location = world_location - location;
			return sprite.get_tile(local_location);
		}
	};

	std::ostream& operator<<(std::ostream& os, const Block& b)
	{
		os << b.get_sprite();
		return os;
	}

	bool do_blocks_overlap(const Block& left, const Block& right)
	{
		const coords offset = left.location - right.location;
		const bool result = do_sprites_overlap(left.sprite, right.sprite, offset);
		return result;
	}

	struct RepeatInfo
	{
		SpriteSheet image_of_top;
		int64_t height_at_top = 0;
		std::size_t wind_idx = 0;
		std::size_t num_blocks_dropped = 0;
	};

	constexpr std::strong_ordering operator<=>(const RepeatInfo& left, const RepeatInfo& right)
	{
		const auto image_match = left.image_of_top <=> right.image_of_top;
		if (image_match == std::weak_ordering::equivalent)
		{
			const SpriteType left_next = get_block_from_idx(left.num_blocks_dropped);
			const SpriteType right_next = get_block_from_idx(right.num_blocks_dropped);
			const auto type_match = left_next <=> right_next;
			if (type_match == std::weak_ordering::equivalent)
			{
				return left.wind_idx <=> right.wind_idx;
			}
			return type_match;
		}
		return image_match;
	}

	class Column
	{
		static constexpr std::size_t GRID_WIDTH = 3;
		std::vector<Sprite> fixed_tiles;
		Block moving_block;
		int64_t highest_point;
		std::size_t next_block_idx;
		void add_next_line()
		{
			fixed_tiles.push_back(get_default_sprite(SpriteType::left_wall));
			fixed_tiles.push_back(get_default_sprite(SpriteType::empty));
			fixed_tiles.push_back(get_default_sprite(SpriteType::right_wall));
		}
		coords get_block_start() const
		{
			const coords result{ STARTING_FROM_LEFT,highest_point + STARTING_HEIGHT };
			return result;
		}
		void spawn_new_block()
		{
			const coords starting_location = get_block_start();
			moving_block.set_location(starting_location);
			moving_block.set_sprite(next_block_idx);
			++next_block_idx;

			auto get_wall_height = [&tiles = fixed_tiles]()
			{
				const int num_tiles = static_cast<int>(tiles.size());
				AdventCheck(num_tiles % GRID_WIDTH == 0);
				const int vertical_blocks = num_tiles / GRID_WIDTH;
				const int height = vertical_blocks * SPRITE_MAX;
				return height;
			};
			
			const int64_t top_y = moving_block.get_location().y + SPRITE_MAX;
			while ((get_wall_height() - SPRITE_MAX) < top_y)
			{
				add_next_line();
			}
		}
		utils::int_range<std::size_t> get_moving_block_idx_range() const
		{
			const int64_t y_location = moving_block.get_location().y;
			const int64_t row_idx = y_location / SPRITE_MAX;
			const std::size_t min_block_idx = row_idx * GRID_WIDTH;
			const std::size_t max_block_idx = min_block_idx + (2 * GRID_WIDTH);
			const utils::int_range result{ min_block_idx,max_block_idx };
			return result;
		}
		Block get_fixed_block_copy(std::size_t idx) const
		{
			AdventCheck(idx < fixed_tiles.size());
			const int64_t y_block = static_cast<int>(idx / GRID_WIDTH);
			const int64_t x_block = static_cast<int>(idx % GRID_WIDTH);
			const coords block_position = coords{ x_block , y_block } *SPRITE_MAX;
			Block block;
			block.set_location(block_position);
			block.set_sprite(fixed_tiles[idx]);
			return block;
		}

		void freeze_moving_block()
		{
			const int64_t dynamic_top = moving_block.get_top_y();
			highest_point = std::max(highest_point, dynamic_top);
			for (std::size_t idx : get_moving_block_idx_range())
			{
				Block block = get_fixed_block_copy(idx);
				block += moving_block;
				fixed_tiles[idx] = block.get_sprite();
			}
		}
		bool try_move_block(utils::direction dir)
		{
			AdventCheck(dir != utils::direction::up);
			const coords offset = coords::dir(dir);
			const coords original_location = moving_block.get_location();
			moving_block.set_location(original_location + offset);
			const auto overlap_range = get_moving_block_idx_range();
			auto overlaps_idx = [this](std::size_t idx)
			{
				const Block fixed_block = get_fixed_block_copy(idx);
				return do_blocks_overlap(moving_block, fixed_block);
			};
			const bool collides = std::any_of(begin(overlap_range), end(overlap_range), overlaps_idx);
			if (collides)
			{
				moving_block.set_location(original_location);
			}
			return !collides;
		}
	public:
		Column()
		{
			next_block_idx = 0;
			highest_point = 0;
			add_next_line();
			AdventCheck(fixed_tiles.size() == 3u);
			fixed_tiles[0] += get_default_sprite(SpriteType::bottom);
			fixed_tiles[1] += get_default_sprite(SpriteType::bottom);
			spawn_new_block();
		}
		
		std::size_t get_number_of_blocks_spawned() const
		{
			return next_block_idx;
		}

		void apply_wind(Wind wind)
		{
			switch (wind)
			{
			case Wind::left:
				try_move_block(utils::direction::left);
				break;
			case Wind::right:
				try_move_block(utils::direction::right);
				break;
			default:
				AdventUnreachable();
				return;
			}

			const bool move_success = try_move_block(utils::direction::down);
			if (!move_success)
			{
				freeze_moving_block();
				spawn_new_block();
			}
		}

		Tile get_tile(const coords& location) const
		{
			const Tile moving = moving_block.get_tile(location);
			if (moving == Tile::block) return Tile::dynamic_block;

			const coords block_coords = location / SPRITE_MAX;
			const std::size_t block_idx = block_coords.y * GRID_WIDTH + block_coords.x;
			const Block target_block = get_fixed_block_copy(block_idx);
			const Tile fixed = target_block.get_tile(location);
			return fixed;
		}
		int64_t get_num_vertical_tiles() const
		{
			return static_cast<int64_t>(fixed_tiles.size() / GRID_WIDTH);
		}
		int64_t get_heighest_point(bool dynamic) const
		{
			if (dynamic)
			{
				const int64_t dynamic_max = moving_block.get_top_y();
				const int64_t result = std::max(highest_point, dynamic_max);
				return result;
			}
			else
			{
				return highest_point;
			};
		}

		RepeatInfo get_repeat_info() const
		{
			RepeatInfo result;
			result.height_at_top = get_heighest_point(false);
			result.num_blocks_dropped = next_block_idx;
			
			const int64_t zero_y = result.height_at_top + 1;
			const int64_t initial_x = [this, zero_y]()
			{
				utils::int_range x_range{ GRID_WIDTH * SPRITE_MAX };
				const auto find_result = std::find_if(begin(x_range), end(x_range),
					[this, zero_y](int64_t x)
					{
						const coords c{ x,zero_y };
						const Tile t = get_tile(c);
						return t != Tile::block;
					});
				return find_result != end(x_range) ? *find_result : int64_t{ -1 };
			}();

			AdventCheck(initial_x >= 0);

			std::vector<coords> search_space{ coords{ initial_x, zero_y } };
			std::size_t idx_to_check = 0;

			while (idx_to_check < search_space.size())
			{
				const coords coords_to_check = search_space[idx_to_check];

				const Tile tile_to_check = get_tile(coords_to_check);
				if (tile_to_check == Tile::block)
				{
					const coords image_space_coords = coords_to_check - coords{ 0,zero_y };
					result.image_of_top.set(Tile::block, image_space_coords, GRID_WIDTH);
				}
				else
				{
					const std::array<coords, 3> search_directions{ coords::left(), coords::down(), coords::right() };
					for (const coords& dir : search_directions)
					{
						const coords next_search_pos = coords_to_check + dir;

						if (std::find(begin(search_space), end(search_space), next_search_pos) == end(search_space))
						{
							search_space.push_back(next_search_pos);
						}
					}
				}
				++idx_to_check;
			}
			return result;
		}
	};

	std::ostream& operator<<(std::ostream& os, const Column& column)
	{
		constexpr bool dynamic_highest_point = true;
		const int64_t height = column.get_heighest_point(dynamic_highest_point);
		for (int64_t h : utils::int_range<int64_t>{ height,-1,-1 })
		{
			os << '\n';
			for (int64_t w : utils::int_range{ 1+7+1 })
			{
				const coords location{ w,h };
				const Tile tile = column.get_tile(location);
				const char out = to_char(tile);
				os << out;
			}
		}
		return os;
	}

	int64_t solve_generic(std::string_view input, std::size_t num_steps)
	{
		utils::sorted_vector<RepeatInfo> previous_states;
		bool should_check_for_repeats = true;
		int64_t height_offset = 0;
		std::size_t blocks_skipped = 0;

		Column column;
		log << "\nInitial state:" << column;
		std::size_t num_blocks_simulated = 1;
		while (true)
		{
			for (std::size_t iwind : utils::int_range{ input.size() })
			{
				const std::size_t num_blocks_so_far = num_blocks_simulated + blocks_skipped;
				const char cwind = input[iwind];
				const Wind wind = to_wind(cwind);
				column.apply_wind(wind);
				log << "\n\nWind direction: " << (wind == Wind::left ? "left" : "right");
				log << column;

				const std::size_t num_blocks_spawned = column.get_number_of_blocks_spawned();
				if (num_blocks_spawned != num_blocks_so_far)
				{
					log << "\nNew block spawned: " << column;
					num_blocks_simulated = num_blocks_spawned;
					if (should_check_for_repeats)
					{
						RepeatInfo current_state = column.get_repeat_info();
						current_state.wind_idx = iwind;
						const auto previous_state_it = previous_states.find(current_state);
						if (previous_state_it != end(previous_states))
						{
							const RepeatInfo& previous_state = *previous_state_it;
							const std::size_t blocks_per_cycle = current_state.num_blocks_dropped - previous_state.num_blocks_dropped;
							const int64_t height_per_cycle = current_state.height_at_top - previous_state.height_at_top;

							const std::size_t num_blocks_remaining = num_steps - num_blocks_spawned;
							const std::size_t num_cycles_remaining = num_blocks_remaining / blocks_per_cycle;

							const std::size_t blocks_to_skip = blocks_per_cycle * num_cycles_remaining;
							const int64_t height_to_add = height_per_cycle * num_cycles_remaining;

							log << "\nFound repeat " << blocks_per_cycle << " blocks and " << height_per_cycle << " height.";
							log << "\nSkipped " << blocks_to_skip << " blocks to add " << height_to_add << " in height.";
							height_offset += height_to_add;
							blocks_skipped += blocks_to_skip;
							should_check_for_repeats = false;
						}
						else
						{
							previous_states.push_back(std::move(current_state));
						}
					}
				}
				if (num_blocks_so_far > num_steps)
				{
					log << "\nFinal column:" << column;
					constexpr bool dynamic_highest_point = false;
					return column.get_heighest_point(dynamic_highest_point) + height_offset;
				}
			}
		}
	}

	int64_t solve_p1(std::string_view input)
	{
		return solve_generic(input, 2022);
	}

	std::string to_string(std::istream& input)
	{
		std::string line;
		line.reserve(10092);
		std::getline(input, line);
		return line;
	}

	int64_t solve_p1(std::istream& input)
	{
		const std::string line = to_string(input);
		return solve_p1(line);
	}
}

namespace
{
	int64_t solve_p2(std::string_view input)
	{
		return solve_generic(input, 1'000'000'000'000);
	}

	int64_t solve_p2(std::istream& input)
	{
		const std::string line = to_string(input);
		return solve_p2(line);;
	}
}

namespace
{
	std::string testcase_a()
	{
		return std::string{
			">>><<><>><<<>><>>><<<>>><<<><<<>><>><<>>"
		};
	}
}

ResultType day_seventeen_p1_a()
{
	auto input = testcase_a();
	return solve_p1(input);
}

ResultType day_seventeen_p2_a()
{
	auto input = testcase_a();
	return solve_p2(input);
}

ResultType advent_seventeen_p1()
{
	auto input = advent::open_puzzle_input(17);
	return solve_p1(input);
}

ResultType advent_seventeen_p2()
{
	auto input = advent::open_puzzle_input(17);
	return solve_p2(input);
}

#undef DAY17DBG
#undef ENABLE_DAY17DBG