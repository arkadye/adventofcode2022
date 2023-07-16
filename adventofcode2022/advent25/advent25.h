#pragma once

#include "../advent/advent_types.h"

namespace day25_internal
{
	ResultType day_twentyfive_p1_std(uint64_t snafuized);
	ResultType day_twentyfive_p1_dts(int decimal);
}

template <uint64_t SNAFUIZED>
ResultType day_twentyfive_p1_std() { return day25_internal::day_twentyfive_p1_std(SNAFUIZED); }

template <int ARG>
ResultType day_twentyfive_p1_dts() { return day25_internal::day_twentyfive_p1_dts(ARG); }

ResultType day_twentyfive_p1_a();

ResultType advent_twentyfive_p1();
ResultType advent_twentyfive_p2();