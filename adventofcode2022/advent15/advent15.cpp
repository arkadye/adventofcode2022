#include "advent15.h"
#include "../advent/advent_utils.h"

#define ENABLE_DAY15DBG 1
#ifdef NDEBUG
#define DAY15DBG 0
#else
#define DAY15DBG ENABLE_DAY15DBG
#endif

#if DAY15DBG
	#include <iostream>
#endif

namespace
{
#if DAY15DBG
	std::ostream & log = std::cout;
#else
	struct {	template <typename T> auto& operator<<(const T&) const noexcept { return *this; } } log;
#endif
}

#include "coords.h"
#include "range_contains.h"
#include "parse_utils.h"
#include "to_value.h"
#include "istream_line_iterator.h"
#include "transform_if.h"
#include "sorted_vector.h"
#include "int_range.h"
#include "sparse_array.h"
#include <vector>

namespace
{
	using utils::coords;

	struct AxisRange
	{
		int low;
		int high;
		AxisRange(int l, int h) : low{ l }, high{ h }
		{
			AdventCheck(l <= h);
		}

		int size() const
		{
			AdventCheck(high >= low);
			return high - low;
		}
		auto operator<=>(const AxisRange&) const = default;
	};

	std::vector<AxisRange> normalize_range_set(std::vector<AxisRange> ranges)
	{
		std::ranges::sort(ranges);
		std::vector<AxisRange> result;
		result.reserve(ranges.size());
		for (AxisRange r : ranges)
		{
			if (r.low == r.high)
			{
				continue;
			}

			if (result.empty())
			{
				result.push_back(r);
				continue;
			}

			const AxisRange previous_r = result.back();
			
			// Guaranteed that r >= previous_range
			if (r.low > previous_r.high)
			{
				result.push_back(r);
				continue;
			}

			if (r.low <= previous_r.high)
			{
				result.back().high = std::max(r.high, previous_r.high);
				continue;
			}
		}

		return result;
	}

	class Sensor
	{
		coords location;
		coords nearest_beacon;
		int range = 0;
	public:
		Sensor(coords my_location, coords my_nearest_beacon)
			: location{ my_location }
			, nearest_beacon{ my_nearest_beacon }
			, range{ my_location.manhatten_distance(my_nearest_beacon) }
		{}

		coords get_location() const { return location; }
		coords get_beacon() const { return nearest_beacon; }
		int get_range() const { return range; }

		AxisRange get_area_covered(int slice) const
		{
			const int y_offset = std::abs(slice - location.y);
			
			AxisRange result{ location.x,location.x };
			const int x_range = range - y_offset;
			if (x_range < 0) return result;

			result.low -= x_range;
			result.high += x_range + 1;
			return result;
		}
	};

	coords parse_coords(std::string_view x_str, std::string_view y_str)
	{
		x_str = utils::remove_specific_prefix(x_str, "x=");
		x_str = utils::remove_specific_suffix(x_str, ',');
		y_str = utils::remove_specific_prefix(y_str, "y=");

		const int x = utils::to_value<int>(x_str);
		const int y = utils::to_value<int>(y_str);
		return coords{ x,y };
	}

	Sensor parse_sensor(std::string_view line)
	{
		auto [sensor_x, sensor_y, beacon_x, beacon_y] = utils::get_string_elements(line, 2, 3, 8, 9);
		sensor_y = utils::remove_specific_suffix(sensor_y, ':');
		const coords sensor_loc = parse_coords(sensor_x, sensor_y);
		const coords beacon_loc = parse_coords(beacon_x, beacon_y);
		const Sensor result{ sensor_loc,beacon_loc };
		return result;
	}

	std::vector<Sensor> parse_all_sensors(std::istream& input, int check_min, int check_max)
	{
		using ILI = utils::istream_line_iterator;
		std::vector<Sensor> result;
		result.reserve(30);
		auto sensor_in_range = [check_min, check_max](const Sensor& s)
		{
			const coords loc = s.get_location();
			const int r = s.get_range();

			const int x_min = loc.x - r;
			if (x_min > check_max) return false;
			
			const int x_max = loc.x + r;
			if (x_max < check_min) return false;

			const int y_min = loc.y - r;
			if (y_min > check_max) return false;

			const int y_max = loc.y + r;
			if (y_max < check_min) return false;

			return true;
		};

		utils::ranges::transform_if_post(utils::istream_line_range{input},std::back_inserter(result),
			parse_sensor, sensor_in_range);
		return result;
	}

	std::vector<Sensor> parse_all_sensors(std::istream& input)
	{
		return parse_all_sensors(input, std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
	}

	std::vector<AxisRange> get_covered_areas(const std::vector<Sensor>& sensors, int y_slice, int min_check, int max_check)
	{
		std::vector<AxisRange> covered_areas;
		covered_areas.reserve(sensors.size());

		std::ranges::transform(sensors, std::back_inserter(covered_areas), [y_slice, min_check, max_check](const Sensor& s)
			{
				AxisRange result = s.get_area_covered(y_slice);
				result.low = std::max(min_check, result.low);
				result.high = std::min(max_check, result.high);
				return result;
			});

		covered_areas = normalize_range_set(std::move(covered_areas));
		return covered_areas;
	}

	std::vector<AxisRange> get_covered_areas(const std::vector<Sensor>& sensors, int y_slice)
	{
		return get_covered_areas(sensors, y_slice, std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
	}

	int get_area_covered(const std::vector<AxisRange>& covered_areas)
	{
		const int result = std::transform_reduce(begin(covered_areas), end(covered_areas), 0, std::plus<int>{},
			[](const AxisRange& ar) { return ar.size(); });
		return result;
	}

	int solve_p1_generic(std::istream& input, int y_slice)
	{
		const std::vector<Sensor> sensors = parse_all_sensors(input);
		utils::sorted_vector<int> beacon_x_coords;
		utils::ranges::transform_if_pre(sensors, std::back_inserter(beacon_x_coords),
			[](const Sensor& s)
			{
				return s.get_beacon().x;
			},
			[y_slice](const Sensor& s)
			{
				return s.get_beacon().y == y_slice;
			});
		beacon_x_coords.unique();
		const std::vector<AxisRange> covered_areas = get_covered_areas(sensors, y_slice);

		const int num_beacons = static_cast<int>(beacon_x_coords.size());
		const int result = get_area_covered(covered_areas) - num_beacons;
		return result;
	}

	int solve_p1(std::istream& input)
	{
		return solve_p1_generic(input, 2000000);
	}
}

namespace
{
	int64_t get_tuning_frequency(int x, int y)
	{
		constexpr int64_t tuning_frequency_multiplier = 4000000;
		const int64_t xl{ x };
		const int64_t yl{ y };
		return xl * tuning_frequency_multiplier + yl;
	}

	int64_t solve_p2_generic(std::istream& input, int max_size)
	{
		const std::vector<Sensor> sensors = parse_all_sensors(input, 0, max_size + 1);
		std::vector<AxisRange> covered_areas;
		for (int y : utils::int_range{ max_size + 1 })
		{
			covered_areas.clear();
			covered_areas = get_covered_areas(sensors, y, 0, max_size + 1);

			if (covered_areas.size() > 1)
			{
				AdventCheck(covered_areas.size() == 2);
				AdventCheck((covered_areas[1].low - covered_areas[0].high) == 1);
				const int x = covered_areas[0].high;
				return get_tuning_frequency(x, y);
			}
		}
		AdventUnreachable();
		return 0;
	}

	int64_t solve_p2(std::istream& input)
	{
		return solve_p2_generic(input, 4000000);
	}
}

ResultType day_fifteen_p1_a()
{
	auto input = advent::open_testcase_input(15,'a');
	return solve_p1_generic(input,10);
}

ResultType day_fifteen_p2_a()
{
	auto input = advent::open_testcase_input(15, 'a');
	return solve_p2_generic(input, 20);
}

ResultType advent_fifteen_p1()
{
	auto input = advent::open_puzzle_input(15);
	return solve_p1(input);
}

ResultType advent_fifteen_p2()
{
	auto input = advent::open_puzzle_input(15);
	return solve_p2(input);
}

#undef DAY15DBG
#undef ENABLE_DAY15DBG