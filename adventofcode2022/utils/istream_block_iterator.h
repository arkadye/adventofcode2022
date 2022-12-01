#pragma once

#include <istream>
#include <string>
#include <string_view>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <cassert>

namespace utils
{

	class istream_block_iterator
	{
	private:
		std::istream* m_stream;
		std::string_view m_sentinental;
		bool is_at_end() const { return m_stream == nullptr; }
	public:
		using pointer = const std::string*;
		using reference = const std::string&;
		using value_type = std::string;
		using difference_type = int;
		using iterator_category = std::input_iterator_tag;
		explicit istream_block_iterator(std::istream& stream, std::string_view sentinental = "") noexcept
			: m_stream{ &stream }, m_sentinental{ sentinental }{}
		istream_block_iterator() noexcept : m_stream{ nullptr }, m_sentinental{}{}
		istream_block_iterator(const istream_block_iterator&) noexcept = default;
		istream_block_iterator& operator=(const istream_block_iterator&) noexcept = default;

		bool operator==(const istream_block_iterator& other) const noexcept
		{
			return is_at_end() && other.is_at_end();
		}

		bool operator!=(const istream_block_iterator& other) const noexcept
		{
			return !(this->operator==(other));
		}

		std::string operator*()
		{
			if (is_at_end())
			{
				throw std::range_error{"Cannot deference an at-the-end stream block iterator"};
			}

			std::ostringstream result_stream;
			while (true)
			{
				std::string line;
				std::getline(*m_stream,line);
				result_stream << line << '\n';
				if (m_stream->eof() || line == m_sentinental)
				{
					break;
				}
			}

			if (m_stream->eof())
			{
				m_stream = nullptr;
			}

			// Get the result and remove the trailing '\n'.
			std::string result = result_stream.str();
			if (!result.empty())
			{
				assert(result.back() == '\n');
				result.pop_back();
			}
			return result;
		}

		istream_block_iterator& operator++() noexcept { return *this; }
		istream_block_iterator  operator++(int) noexcept { return *this; }
	};

	class stream_block_range
	{
		std::istream& stream;
	public:
		explicit stream_block_range(std::istream& input) : stream{ input } {}
		stream_block_range(const stream_block_range& other) = default;
		stream_block_range() = delete;
		istream_block_iterator begin() const { return istream_block_iterator{ stream }; }
		istream_block_iterator end() const { return istream_block_iterator{}; }
	};
}

inline utils::istream_block_iterator begin(utils::stream_block_range lr) { return lr.begin(); }
inline utils::istream_block_iterator end(utils::stream_block_range lr) { return lr.end(); }