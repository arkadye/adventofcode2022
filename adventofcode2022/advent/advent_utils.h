#pragma once

#include <fstream>
#include <sstream>
#include <cassert>

#include "advent_assert.h"

namespace advent
{

	inline std::ifstream open_puzzle_input(int day)
	{
		std::ostringstream name;
		name << "advent" << day << "/advent" << day << ".txt";
		auto result = std::ifstream{ name.str() };
		AdventCheck(result.is_open());
		return result;
	}

	inline std::ifstream open_testcase_input(int day, char id)
	{
		std::ostringstream name;
		name << "advent" << day << "/testcase_" << id << ".txt";
		auto result = std::ifstream{ name.str() };
		AdventCheck(result.is_open());
		return result;
	}
}