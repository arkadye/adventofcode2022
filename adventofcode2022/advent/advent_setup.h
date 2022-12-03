#pragma once

#include "advent_testcase_setup.h"

static const std::string DEFAULT_FILTER = "_two_";

static const verification_test tests[] =
{
	TESTCASE(day_one_p1_a,24000),
	TESTCASE(day_one_p2_a,45000),
	DAY(one,70698,206643),
	TESTCASE(day_two_p1_a,15),
	TESTCASE(day_two_p2_a,12),
	DAY(two,14375,Dummy{}),
	DAY(three,Dummy{},Dummy{}),
	DAY(four,Dummy{},Dummy{}),
	DAY(five,Dummy{},Dummy{}),
	DAY(six,Dummy{},Dummy{}),
	DAY(seven,Dummy{},Dummy{}),
	DAY(eight,Dummy{},Dummy{}),
	DAY(nine,Dummy{},Dummy{}),
	DAY(ten,Dummy{},Dummy{}),
	DAY(eleven,Dummy{},Dummy{}),
	DAY(twelve,Dummy{},Dummy{}),
	DAY(thirteen,Dummy{},Dummy{}),
	DAY(fourteen,Dummy{},Dummy{}),
	DAY(fifteen,Dummy{},Dummy{}),
	DAY(sixteen,Dummy{},Dummy{}),
	DAY(seventeen,Dummy{},Dummy{}),
	DAY(eighteen,Dummy{},Dummy{}),
	DAY(nineteen,Dummy{},Dummy{}),
	DAY(twenty,Dummy{},Dummy{}),
	DAY(twentyone,Dummy{},Dummy{}),
	DAY(twentytwo,Dummy{},Dummy{}),
	DAY(twentythree,Dummy{},Dummy{}),
	DAY(twentyfour,Dummy{},Dummy{}),
	DAY(twentyfive,Dummy{},"MERRY CHRISTMAS!")
};

#undef ARG
#undef TESTCASE
#undef FUNC_NAME
#undef TEST_DECL
#undef DAY
#undef DUMMY
#undef DUMMY_DAY