#pragma once

#include <string_view>
#include <stdexcept>
#include <optional>
#include "split_string.h"

#include "../advent/advent_assert.h"

namespace utils
{

	class string_line_iterator
	{
	private:
		std::string_view m_string;
		char m_sentinental;
		std::optional<std::pair<std::string_view,std::string_view>> m_cached_split_result;
		std::pair<std::string_view, std::string_view> get_split_result()
		{
			if (is_at_end())
			{
				throw std::range_error{ "Cannot dereference or increment an at-the-end string line iterator" };
			}

			if (!m_cached_split_result.has_value())
			{
				m_cached_split_result = utils::split_string_at_first(m_string,m_sentinental);
			}

			AdventCheck(m_cached_split_result.has_value());
			return m_cached_split_result.value();
		}
		bool is_at_end() const { return m_string.empty(); }
	public:
		using pointer = const std::string*;
		using reference = const std::string&;
		using value_type = std::string;
		using difference_type = int;
		using iterator_category = std::forward_iterator_tag;
		explicit string_line_iterator(std::string_view string, char sentinental = '\n') noexcept
			: m_string{ string }, m_sentinental{ sentinental }{}
		string_line_iterator() noexcept : m_string{ "" }, m_sentinental{ 0 }{}
		string_line_iterator(const string_line_iterator&) noexcept = default;
		string_line_iterator& operator=(const string_line_iterator&) noexcept = default;

		bool operator==(const string_line_iterator& other) const noexcept
		{
			return is_at_end() && other.is_at_end();
		}

		bool operator!=(const string_line_iterator& other) const noexcept
		{
			return !(this->operator==(other));
		}

		std::string_view operator*()
		{
			auto [result,unneeded] = get_split_result();
			return result;
		}

		string_line_iterator& operator++()
		{
			auto [unneeded,new_string] = get_split_result();
			m_string = new_string;
			m_cached_split_result.reset();
			return *this;
		}
		string_line_iterator  operator++(int)
		{
			string_line_iterator result = *this;
			operator++();
			return result;
		}
	};

	class string_line_range
	{
		std::string_view stream;
		char delim = '\0';
	public:
		explicit string_line_range(std::string_view input, char sentinental = '\n') : stream{ input }, delim{ sentinental } {}
		string_line_range(const string_line_range& other) = default;
		string_line_range() = delete;
		string_line_iterator begin() const { return string_line_iterator{ stream ,delim }; }
		string_line_iterator end() const { return string_line_iterator{}; }
	};
}

inline utils::string_line_iterator begin(utils::string_line_range lr) { return lr.begin(); }
inline utils::string_line_iterator end(utils::string_line_range lr) { return lr.end(); }