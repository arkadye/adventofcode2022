#pragma once

#include <string>
#include <variant>
#include <cstdint>

using ResultType = std::variant<std::string, int64_t>;
enum class AdventDay
{
	One,
	Two
};