#include "advent13.h"
#include "../advent/advent_utils.h"

#define ENABLE_DAY13DBG 1
#ifdef NDEBUG
#define DAY13DBG 0
#else
#define DAY13DBG ENABLE_DAY13DBG
#endif

#if DAY13DBG
	#include <iostream>
#endif

namespace
{
#if DAY13DBG
	std::ostream & log = std::cout;
#else
	struct {	template <typename T> auto& operator<<(const T&) const noexcept { return *this; } } log;
#endif
}

#include "istream_block_iterator.h"
#include "small_vector.h"
#include "brackets.h"
#include "split_string.h"
#include "parse_utils.h"
#include "to_value.h"

#include <variant>
#include <optional>

namespace
{
	using Value_t = int;

	constexpr char OPEN_BRACKET = '[';
	constexpr char CLOSE_BRACKET = ']';
	constexpr char LIST_DELIM = ',';

	// If list, start and end with [ and ]. Otherwise must pass utils::is_value.
	struct Packet_Base
	{
		std::string_view data;
		explicit Packet_Base(std::string_view init) : data{init}{}
		Packet_Base(const Packet_Base&) = default;
	};

	// data must pass utils::is_value.
	struct Packet_AsValue
	{
		std::string_view data;
		explicit Packet_AsValue(std::string_view init) : data{ init }
		{
			AdventCheck(utils::is_value(data));
		}
		explicit Packet_AsValue(Packet_Base other) : Packet_AsValue{other.data}{}
		Packet_AsValue(const Packet_AsValue&) = default;
	};

	auto begin(Packet_AsValue packet) { return begin(packet.data); }
	auto end(Packet_AsValue packet) {return end(packet.data); }

	// Do NOT include brackets at start and end.
	struct Packet_AsList
	{
	private:
		static std::string_view get_from_base_init(Packet_Base base)
		{
			base.data = utils::remove_specific_prefix(base.data,OPEN_BRACKET);
			base.data = utils::remove_specific_suffix(base.data,CLOSE_BRACKET);
			return base.data;
		}
	public:
		std::string_view data;
		explicit Packet_AsList(std::string_view init) : data{init}{}
		explicit Packet_AsList(Packet_Base other) : Packet_AsList{get_from_base_init(other)} {}
		explicit Packet_AsList(Packet_AsValue other) : Packet_AsList{other.data}{}
		Packet_AsList(const Packet_AsList& other) = default;
		bool is_at_end() const { return data.empty(); }
	};

	class Packet_AsList_Iterator
	{
		Packet_AsList base;
		mutable std::optional<Packet_Base> deref_bit;
		mutable std::optional<Packet_AsList> after_data;
		void split() const
		{
			if(deref_bit.has_value()) return;
			const std::size_t split_point = utils::bracket_aware_find(base.data,OPEN_BRACKET,CLOSE_BRACKET,LIST_DELIM);
			const auto [deref,after] = utils::split_string_at_point(base.data,split_point);
			deref_bit = Packet_Base{deref};
			after_data = Packet_AsList{after};
		}
	public:
		Packet_AsList_Iterator(Packet_AsList input) : base{input}{}
		Packet_AsList_Iterator() : base{""}{}
		Packet_Base operator*() const { split(); return deref_bit.value(); }
		Packet_AsList_Iterator& operator++()
		{
			split();
			base = after_data.value();
			return *this;
		}
		Packet_AsList_Iterator operator++(int)
		{
			Packet_AsList_Iterator result = *this;
			++(*this);
			return result;
		}
		bool operator==(const Packet_AsList_Iterator& other) const { return this->base.data == other.base.data; }
		bool operator!=(const Packet_AsList_Iterator& other) const { return !operator==(other); }
	};

	Packet_AsList_Iterator begin(Packet_AsList packet) {return Packet_AsList_Iterator{packet}; }
	Packet_AsList_Iterator end(Packet_AsList packet) { return Packet_AsList_Iterator{}; }

	using Packet = std::variant<Packet_Base,Packet_AsList,Packet_AsValue>;

	Packet make_packet(std::string_view input)
	{
		return Packet_Base{input};
	}

	struct ToPacket
	{
		Packet operator()(Packet_Base base) const
		{
			AdventCheck(!base.data.empty());
			if (base.data.front() == OPEN_BRACKET)
			{
				return Packet_AsList{ base };
			}
			else
			{
				return Packet_AsValue{ base };
			}
			AdventUnreachable();
			return base;
		}
		Packet operator()(Packet_AsList base) const
		{
			return base;
		}
		Packet operator()(Packet_AsValue base) const
		{
			return base;
		}
	};

	Packet normalize(Packet packet)
	{
		return std::visit(ToPacket{},packet);
	}

	struct CompareThreeWay
	{
		template <typename InType>
		std::strong_ordering generic_compare(InType left, InType right) const
		{
			auto l_b = begin(left);
			auto l_e = end(left);
			auto r_b = begin(right);
			auto r_e = end(right);
			while (true)
			{
				if (l_b == l_e)
				{
					return r_b == r_e ? std::strong_ordering::equal : std::strong_ordering::less;
				}

				if(r_b == r_e) return std::strong_ordering::greater;
				const auto l_arg = *l_b;
				const auto r_arg = *r_b;
				const std::strong_ordering result = operator()(l_arg,r_arg);
				if (result != std::strong_ordering::equal)
				{
					return result;
				}
				++l_b;
				++r_b;
			}
		}

		std::strong_ordering normalize_and_compare(Packet left, Packet right) const
		{
			left = normalize(left);
			right = normalize(right);
			return std::visit(CompareThreeWay{},left,right);
		}

		std::strong_ordering operator()(char left, char right) const
		{
			return left <=> right;
		}
		std::strong_ordering operator()(Packet_Base left, Packet_Base right) const
		{
			return normalize_and_compare(left,right);
		}
		std::strong_ordering operator()(Packet_Base left, Packet_AsList right) const
		{
			return normalize_and_compare(left,right);
		}
		std::strong_ordering operator()(Packet_Base left, Packet_AsValue right) const
		{
			return normalize_and_compare(left,right);
		}
		std::strong_ordering operator()(Packet_AsList left, Packet_Base right) const
		{
			return normalize_and_compare(left,right);
		}
		std::strong_ordering operator()(Packet_AsList left, Packet_AsList right) const
		{
			return generic_compare(left,right);
		}
		std::strong_ordering operator()(Packet_AsList left, Packet_AsValue right) const
		{
			Packet_AsList right_list{right};
			return operator()(left,right_list);
		}
		std::strong_ordering operator()(Packet_AsValue left, Packet_Base right) const
		{
			return normalize_and_compare(left,right);
		}
		std::strong_ordering operator()(Packet_AsValue left, Packet_AsList right) const
		{
			Packet_AsList left_list{left};
			return operator()(left_list,right);
		}
		std::strong_ordering operator()(Packet_AsValue left, Packet_AsValue right) const
		{
			return generic_compare(left.data,right.data);
		}
	};

	bool are_packets_in_order(Packet left, Packet right)
	{
		const std::strong_ordering order = std::visit(CompareThreeWay{},left,right);
		return order != std::strong_ordering::greater;
	}

	bool are_packets_in_order(std::string_view left, std::string_view right)
	{
		return are_packets_in_order(make_packet(left),make_packet(right));
	}

	bool are_packets_in_order(std::string_view line_pair)
	{
		AdventCheck(std::count(begin(line_pair),end(line_pair),'\n') == 1);
		auto [left,right] = utils::split_string_at_first(line_pair,'\n');
		return are_packets_in_order(left,right);
	}

	int solve_p1(std::istream& input)
	{
		int result = 0;
		int idx = 1;
		for (auto block : utils::istream_block_range{ input })
		{
			if (are_packets_in_order(block))
			{
				result += idx;
			}
			++idx;
		}
		return result;
	}
}

namespace
{
	int solve_p2(std::istream& input)
	{
		return 0;
	}
}

namespace
{
	std::istringstream testcase_a()
	{
		return std::istringstream{
			"[1,1,3,1,1]\n"
			"[1,1,5,1,1]\n"
			"\n"
			"[[1],[2,3,4]]\n"
			"[[1],4]\n"
			"\n"
			"[9]\n"
			"[[8,7,6]]\n"
			"\n"
			"[[4,4],4,4]\n"
			"[[4,4],4,4,4]\n"
			"\n"
			"[7,7,7,7]\n"
			"[7,7,7]\n"
			"\n"
			"[]\n"
			"[3]\n"
			"\n"
			"[[[]]]\n"
			"[[]]\n"
			"\n"
			"[1,[2,[3,[4,[5,6,7]]]],8,9]\n"
			"[1,[2,[3,[4,[5,6,0]]]],8,9]"
		};
	}
}

ResultType day_thirteen_p1_a()
{
	auto input = testcase_a();
	return solve_p1(input);
}

ResultType advent_thirteen_p1()
{
	auto input = advent::open_puzzle_input(13);
	return solve_p1(input);
}

ResultType advent_thirteen_p2()
{
	auto input = advent::open_puzzle_input(13);
	return solve_p2(input);
}

#undef DAY13DBG
#undef ENABLE_DAY13DBG