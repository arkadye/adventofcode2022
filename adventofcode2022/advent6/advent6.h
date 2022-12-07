#pragma once

#include "../advent/advent_types.h"

namespace day_six_internal
{
	ResultType day_six_p1_testcase(std::size_t testcase_idx);
	ResultType day_six_p2_testcase(std::size_t testcase_idx);
}

template <std::size_t TESTCASE_IDX>
inline ResultType day_six_p1_testcase() { return day_six_internal::day_six_p1_testcase(TESTCASE_IDX); }

template <std::size_t TESTCASE_IDX>
inline ResultType day_six_p2_testcase() { return day_six_internal::day_six_p2_testcase(TESTCASE_IDX); }

ResultType advent_six_p1();
ResultType advent_six_p2();