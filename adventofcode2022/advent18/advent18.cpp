#include "advent18.h"
#include "../advent/advent_utils.h"

#define ENABLE_DAY18DBG 1
#ifdef NDEBUG
#define DAY18DBG 0
#else
#define DAY18DBG ENABLE_DAY18DBG
#endif

#if DAY18DBG
	#include <iostream>
#endif

namespace
{
#if DAY18DBG
	std::ostream & log = std::cout;
#else
	struct {	template <typename T> auto& operator<<(const T&) const noexcept { return *this; } } log;
#endif
}

#include "sorted_vector.h"
#include "position3d.h"
#include "to_value.h"
#include "parse_utils.h"
#include "int_range.h"
#include "range_contains.h"
#include "istream_line_iterator.h"

#include <array>

namespace
{
	using CoordValueType = int8_t;
	using Coords = utils::Vector3D<CoordValueType>;
	struct Droplet
	{
		Coords coords;
		int8_t faces_exposed;
		constexpr auto operator<=>(const Droplet& other) const
		{
			return coords <=> other.coords;
		}
		explicit Droplet(const Coords& in_coords) : coords{ in_coords }, faces_exposed{ 6 } {}
		Droplet(int x, int y, int z) : Droplet{ Coords{x,y,z} } {}
		Droplet() : Droplet{ 0,0,0 } {}
		explicit Droplet(std::string_view sv)
		{
			faces_exposed = 6;

			const auto [x, y, z] = utils::get_string_elements(sv, ',', 0, 1, 2);
			std::array<std::string_view, 3> in_array = { x,y,z };
			for (std::size_t i : utils::int_range{ std::size_t{3} })
			{
				const CoordValueType val = utils::to_value<CoordValueType>(in_array[i]);
				AdventCheck(utils::range_contains_exc<CoordValueType>(val, 0, 100));
				coords[i] = val;
			}
		}
	};

	using DropletMap = utils::sorted_vector<Droplet>;

	Droplet* get_droplet(DropletMap& map, const Coords& location)
	{
		const auto find_result = map.find(Droplet{ location });
		return find_result != end(map) ? &(*find_result) : nullptr;
	}

	DropletMap parse_droplet_map(std::istream& input, int8_t initial_num_faces)
	{
		DropletMap result;
		result.reserve(2694);
		
		using ILI = utils::istream_line_iterator;
		std::transform(ILI{ input }, ILI{}, std::back_inserter(result),
			[initial_num_faces](std::string_view line)
			{
				Droplet droplet{ line };
				droplet.faces_exposed = initial_num_faces;
				return droplet;
			});
		return result;
	}

	DropletMap set_exposed_faces_p1(DropletMap droplets)
	{
		std::for_each(begin(droplets), end(droplets), [](const Droplet& d)
			{
				AdventCheck(d.faces_exposed == 6);
			});

		std::array<Coords, 3> offsets = { Coords::forward(), Coords::up(), Coords::right() };
		
		for (Droplet& droplet : droplets)
		{
			for (const Coords& offset : offsets)
			{
				const Coords place_to_check = droplet.coords + offset;
				Droplet* found_droplet_ptr = get_droplet(droplets, place_to_check);
				if (found_droplet_ptr != nullptr)
				{
					Droplet& found_droplet = *found_droplet_ptr;
					--found_droplet.faces_exposed;
					--droplet.faces_exposed;
					AdventCheck(utils::range_contains_inc(found_droplet.faces_exposed, 0, 6));
					AdventCheck(utils::range_contains_inc(droplet.faces_exposed, 0, 6));
				}
			}
		}
		return droplets;
	}

	int count_exposed_faces(const DropletMap& droplets)
	{
		auto get_faces = [](const Droplet& d)
		{
			return d.faces_exposed;
		};

		const int result = std::transform_reduce(begin(droplets), end(droplets), 0, std::plus<int>{}, get_faces);
		return result;
	}

	int solve_p1(std::istream& input)
	{
		DropletMap droplets = parse_droplet_map(input, 6);
		droplets = set_exposed_faces_p1(std::move(droplets));
		const int result = count_exposed_faces(droplets);
		return result;
	}
}

namespace
{
	struct AxisBounds
	{
		CoordValueType min, max;
		AxisBounds() : min{ std::numeric_limits<CoordValueType>::max() }
					 , max{ std::numeric_limits<CoordValueType>::min() }
		{}
		void set_value(CoordValueType val)
		{
			min = std::min(min, val);
			max= std::max(max, val);
		}
		bool contains_value(CoordValueType val) const
		{
			return utils::range_contains_inc(val, min, max);
		}
	};

	struct VolumeBounds
	{
		AxisBounds x, y, z;
		VolumeBounds() = default;
		VolumeBounds(const DropletMap& droplets) : VolumeBounds()
		{
			for (const Droplet& droplet : droplets)
			{
				using utils::Axis;
				const Coords& location = droplet.coords;
				x.set_value(location[Axis::X]);
				y.set_value(location[Axis::Y]);
				z.set_value(location[Axis::Z]);
			}
		}
		bool contains_point(const Coords& point) const
		{
			using utils::Axis;
			return x.contains_value(point[Axis::X])
				&& y.contains_value(point[Axis::Y])
				&& z.contains_value(point[Axis::Z]);
		}
	};

	DropletMap set_exposed_faces_p2(DropletMap droplets)
	{
		const VolumeBounds bounds = [&droplets]()
		{
			VolumeBounds smaller{ droplets };
			--smaller.x.min;
			--smaller.y.min;
			--smaller.z.min;
			++smaller.x.max;
			++smaller.y.max;
			++smaller.z.max;
			return smaller;
		}();

		const std::array<Coords, 6> offsets{
			Coords::up(),
			Coords::down(),
			Coords::left(),
			Coords::right(),
			Coords::forward(),
			Coords::backward()
		};

		std::vector<Coords> locations_to_check = {
			Coords{bounds.x.min, bounds.y.min, bounds.z.min},
			Coords{bounds.x.min, bounds.y.min, bounds.z.max},
			Coords{bounds.x.min, bounds.y.max, bounds.z.min},
			Coords{bounds.x.min, bounds.y.max, bounds.z.max},
			Coords{bounds.x.max, bounds.y.min, bounds.z.min},
			Coords{bounds.x.max, bounds.y.min, bounds.z.max},
			Coords{bounds.x.max, bounds.y.max, bounds.z.min},
			Coords{bounds.x.max, bounds.y.max, bounds.z.max}
		};

		utils::sorted_vector<Coords> checked_locations;

		while (!locations_to_check.empty())
		{
			std::vector<Coords> next_coords;
			next_coords.reserve(3 * locations_to_check.size());
			for (const Coords& location : locations_to_check)
			{
				AdventCheck(get_droplet(droplets, location) == nullptr);

				if (checked_locations.contains(location)) continue;
				checked_locations.push_back(location);

				for (const Coords& offset : offsets)
				{
					const Coords next_location = location + offset;
					if (!bounds.contains_point(next_location)) continue;

					Droplet* droplet_ptr = get_droplet(droplets, next_location);
					if (droplet_ptr != nullptr)
					{
						Droplet& droplet = *droplet_ptr;
						++droplet.faces_exposed;
					}
					else
					{
						next_coords.push_back(next_location);
					}
				}
			}
			locations_to_check = std::move(next_coords);
		}

		return droplets;
	}

	int solve_p2(std::istream& input)
	{
		DropletMap droplets = parse_droplet_map(input, 0);
		droplets = set_exposed_faces_p2(std::move(droplets));
		const int result = count_exposed_faces(droplets);
		return result;
	}
}

namespace
{
	std::istringstream testcase_a()
	{
		return std::istringstream{
			"1,1,1\n2,1,1"
		};
	}

	std::istringstream testcase_b()
	{
		return std::istringstream{
			"2,2,2\n"
			"1,2,2\n"
			"3,2,2\n"
			"2,1,2\n"
			"2,3,2\n"
			"2,2,1\n"
			"2,2,3\n"
			"2,2,4\n"
			"2,2,6\n"
			"1,2,5\n"
			"3,2,5\n"
			"2,1,5\n"
			"2,3,5"
		};
	}
}

ResultType day_eighteen_p1_a()
{
	auto input = testcase_a();
	return solve_p1(input);
}

ResultType day_eighteen_p1_b()
{
	auto input = testcase_b();
	return solve_p1(input);
}

ResultType day_eighteen_p2_a()
{
	auto input = testcase_a();
	return solve_p2(input);
}

ResultType day_eighteen_p2_b()
{
	auto input = testcase_b();
	return solve_p2(input);
}

ResultType advent_eighteen_p1()
{
	auto input = advent::open_puzzle_input(18);
	return solve_p1(input);
}

ResultType advent_eighteen_p2()
{
	auto input = advent::open_puzzle_input(18);
	return solve_p2(input);
}

#undef DAY18DBG
#undef ENABLE_DAY18DBG