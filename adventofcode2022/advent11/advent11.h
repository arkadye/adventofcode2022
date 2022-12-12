#pragma once

#include "../advent/advent_types.h"

namespace day_11_internal
{
	ResultType day_eleven_p2_a_generic(int num_rounds);
}

template <int NUMROUNDS>
ResultType day_eleven_p2_a() { return day_11_internal::day_eleven_p2_a_generic(NUMROUNDS); }

ResultType day_eleven_p1_a();

ResultType advent_eleven_p1();
ResultType advent_eleven_p2();