#pragma once

#include <istream>
#include <string>
#include <iterator>
#include <stdexcept>

namespace utils
{

	class istream_line_iterator
	{
	private:
		std::istream* m_stream;
		char m_sentinental;
		bool is_at_end() const { return m_stream == nullptr; }
	public:
		using pointer = const std::string*;
		using reference = const std::string&;
		using value_type = std::string;
		using difference_type = int;
		using iterator_category = std::input_iterator_tag;
		explicit istream_line_iterator(std::istream& stream, char sentinental = '\n') noexcept
			: m_stream{ &stream },  m_sentinental{ sentinental }{}
		istream_line_iterator() noexcept : m_stream{ nullptr }, m_sentinental{ 0 }{}
		istream_line_iterator(const istream_line_iterator&) noexcept = default;
		istream_line_iterator& operator=(const istream_line_iterator&) noexcept = default;

		bool operator==(const istream_line_iterator& other) const noexcept
		{
			return is_at_end() && other.is_at_end();
		}

		bool operator!=(const istream_line_iterator& other) const noexcept
		{
			return !(this->operator==(other));
		}

		std::string operator*()
		{
			if (is_at_end())
			{
				throw std::range_error{"Cannot dereference an at-the-end stream line iterator"};
			}
			std::string result;
			std::getline(*m_stream, result, m_sentinental);
			if (m_stream->eof())
			{
				m_stream = nullptr;
			}
			return result;
		}

		istream_line_iterator& operator++() noexcept { return *this; }
		istream_line_iterator  operator++(int) noexcept { return *this; }
	};

	class stream_line_range
	{
		std::istream& stream;
	public:
		explicit stream_line_range(std::istream& input) : stream{ input } {}
		stream_line_range(const stream_line_range& other) = default;
		stream_line_range() = delete;
		istream_line_iterator begin() const { return istream_line_iterator{ stream }; }
		istream_line_iterator end() const { return istream_line_iterator{}; }
	};
}

inline utils::istream_line_iterator begin(utils::stream_line_range lr) { return lr.begin(); }
inline utils::istream_line_iterator end(utils::stream_line_range lr) { return lr.end(); }