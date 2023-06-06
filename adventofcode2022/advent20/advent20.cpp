#include "advent20.h"
#include "../advent/advent_utils.h"

#define ENABLE_DAY20DBG 1
#ifdef NDEBUG
#define DAY20DBG 0
#else
#define DAY20DBG ENABLE_DAY20DBG
#endif

#if DAY20DBG
	#include <iostream>
#endif

namespace
{
#if DAY20DBG
	std::ostream & log = std::cout;
#else
	struct {	template <typename T> auto& operator<<(const T&) const noexcept { return *this; } } log;
#endif
}

#include <vector>
#include <array>
#include <algorithm>
#include "int_range.h"
#include "range_contains.h"
#include "istream_line_iterator.h"
#include "to_value.h"

namespace
{
	using ValType = int64_t;
	using IdxType = uint16_t;
	constexpr std::size_t max_msg_len = std::numeric_limits<IdxType>::max();

	struct Entry
	{
		ValType base = 0;
		ValType val = 0;
		IdxType next_idx = 0;
		IdxType prev_idx = 0;
	};

	using MessageType = std::vector<Entry>;
	IdxType size(const MessageType& msg)
	{
		const std::size_t len = msg.size();
		AdventCheck(len <= max_msg_len);
		return static_cast<IdxType>(len);
	}

	utils::int_range<IdxType> message_range(const MessageType& message)
	{
		const IdxType len = size(message);
		return utils::int_range{ len};
	}

	std::string to_string(const MessageType& message)
	{
#if DAY20DBG
		if (message.empty())
		{
			return "[Empty]";
		}
		std::ostringstream oss;
		IdxType current_idx = 0;
		do
		{
			oss << message[current_idx].base << ", ";
			current_idx = message[current_idx].next_idx;
		} while (current_idx != 0);
		std::string result = oss.str();
		result.pop_back();
		result.pop_back();
		return result;
#else
		return "";
#endif
	}

	ValType normalise_value(ValType val, IdxType message_len)
	{
		auto normalise_positive = [message_len](ValType val)
		{
			AdventCheck(val > 0);
			
			const ValType vml{ message_len };
			ValType result = val % (vml - 1);
			if (result > (vml / 2))
			{
				result -= (message_len - 1);
			}
			return result;
		};

		if (val == 0) return val;
		else if (val > 0)
		{
			const ValType result = normalise_positive(val);
			return result;
		}
		else // if (val < 0)
		{
			AdventCheck(val < 0);
			const ValType positive_val = -val;
			const ValType positive_result = normalise_positive(positive_val);
			const ValType result = -positive_result;
			return result;
		}
	}

	MessageType get_message(std::istream& input, ValType decryption_key)
	{
		auto decrypt_value = [decryption_key](std::string_view line)
		{
			Entry result;
			const ValType val = utils::to_value<ValType>(line);
			result.val = val * decryption_key;
			return result;
		};

		using ItType = utils::istream_line_iterator;

		MessageType result;
		std::transform(ItType{ input }, ItType{}, std::back_inserter(result), decrypt_value);

		constexpr IdxType zero_idx{ 0 };
		const IdxType message_len = size(result);
		const IdxType max_idx = message_len - 1;
		for (IdxType i : message_range(result))
		{
			Entry& e = result[i];
			e.base = e.val;
			e.val = normalise_value(e.val, message_len);
			e.prev_idx = (i == zero_idx ? max_idx : i - 1);
			e.next_idx = (i == max_idx) ? zero_idx : i + 1;
		}

		return result;
	}

	MessageType mix_once(MessageType message)
	{
		for (IdxType i : message_range(message))
		{
			Entry& this_entry = message[i];

			if (this_entry.val == 0)
			{
				//log << "\nHandling value " << this_entry.base << ": no change to the list.";
				continue;
			}

			const IdxType original_previous_idx = this_entry.prev_idx;
			const IdxType original_next_idx = this_entry.next_idx;

			// Remove from the current position
			message[original_previous_idx].next_idx = original_next_idx;
			message[original_next_idx].prev_idx = original_previous_idx;

			// Find the next one
			if (this_entry.val < 0)
			{
				IdxType previous_entry_idx = original_previous_idx;
				for (auto dummy : utils::int_range{ -this_entry.val })
				{
					previous_entry_idx = message[previous_entry_idx].prev_idx;
				}
				Entry& new_previous = message[previous_entry_idx];
				Entry& new_next = message[new_previous.next_idx];
				this_entry.prev_idx = previous_entry_idx;
				this_entry.next_idx = new_previous.next_idx;
				new_previous.next_idx = i;
				new_next.prev_idx = i;
			}
			else // (this_entry.val > 0)
			{
				AdventCheck(this_entry.val > 0);
				IdxType next_entry_idx = original_next_idx;
				for (auto dummy : utils::int_range{ this_entry.val })
				{
					next_entry_idx = message[next_entry_idx].next_idx;
				}
				Entry& new_next = message[next_entry_idx];
				Entry& new_previous = message[new_next.prev_idx];
				this_entry.prev_idx = new_next.prev_idx;
				this_entry.next_idx = next_entry_idx;
				new_previous.next_idx = i;
				new_next.prev_idx = i;
			}
			//log << "\nHandling value " << this_entry.base
			//	<< ". (Move " << this_entry.val << " spaces). New list:\n" << to_string(message);
		}

		return message;
	}

	MessageType mix(MessageType message, int num_times)
	{
		log << "\nOriginal message:\n" << to_string(message);
		for (auto i : utils::int_range{ num_times })
		{
			message = mix_once(std::move(message));
			log << "\nAfter " << (i + 1) << " round(s) of mixing:\n" << to_string(message);
		}
		return message;
	}

	template <typename Range>
	ValType get_coordinates_generic(const MessageType& message, Range indices)
	{
		auto normalise_idx = [len = size(message)](IdxType idx)
		{
			return idx % len;
		};
		std::transform(begin(indices), end(indices), begin(indices), normalise_idx);
		std::ranges::sort(indices);
		AdventCheck(message.size() < max_msg_len);
		const IdxType msg_len = size(message);
		auto is_start = [](const Entry& e) { return e.val == 0; };
		auto search_spot = std::ranges::find_if(message, is_start);

		IdxType current_idx = 0;
		for (; current_idx < msg_len; ++current_idx)
		{
			const Entry& current = message[current_idx];
			if (current.val == 0) break;
		}
		AdventCheck(current_idx < msg_len);
		
		ValType result = 0;
		IdxType it_idx = 0;
		for (IdxType target_idx : indices)
		{
			while (it_idx <= target_idx)
			{
				const Entry& e = message[current_idx];
				if (it_idx == target_idx)
				{
					result += e.base;
					break;
				}
				else
				{
					current_idx = e.next_idx;
					++it_idx;
				}
			}
		}
		return result;
	}

	ValType get_grove_coordinates(const MessageType& message)
	{
		return get_coordinates_generic(message, std::array<IdxType,3>{1000, 2000, 3000});
	}

	ValType solve_generic(std::istream& input, ValType decryption_key, int num_times_to_mix)
	{
		MessageType message = get_message(input, decryption_key);
		message = mix(std::move(message), num_times_to_mix);
		const ValType result = get_grove_coordinates(message);
		return result;
	}

	ValType solve_p1(std::istream& input)
	{
		const ValType result = solve_generic(input, 1, 1);
		return result;
	}

	ValType solve_p2(std::istream& input)
	{
		constexpr ValType decryption_key = 811589153;
		constexpr int num_times_to_mix = 10;
		const ValType result = solve_generic(input, 811589153, num_times_to_mix);
		return result;
	}
}

namespace
{
	std::istringstream testcase_a()
	{
		return std::istringstream{
			"1\n2\n-3\n3\n-2\n0\n4"
		};
	}
}

ResultType day_twenty_p1_a()
{
	auto input = testcase_a();
	return solve_p1(input);
}

ResultType day_twenty_p2_a()
{
	auto input = testcase_a();
	return solve_p2(input);
}

ResultType advent_twenty_p1()
{
	auto input = advent::open_puzzle_input(20);
	return solve_p1(input);
}

ResultType advent_twenty_p2()
{
	auto input = advent::open_puzzle_input(20);
	return solve_p2(input);
}

#undef DAY20DBG
#undef ENABLE_DAY20DBG