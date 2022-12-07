#pragma once

#include "advent_testcase_setup.h"

static const std::string DEFAULT_FILTER = "_six_";

static const verification_test tests[] =
{
	TESTCASE(day_one_p1_a,24000),
	TESTCASE(day_one_p2_a,45000),
	DAY(one,70698,206643),
	TESTCASE(day_two_p1_a,15),
	TESTCASE(day_two_p2_a,12),
	DAY(two,14375,10274),
	TESTCASE(day_three_p1_a,157),
	TESTCASE(day_three_p2_a,70),
	DAY(three,7691,2508),
	TESTCASE(day_four_p1_a,2),
	TESTCASE(day_four_p2_a,4),
	DAY(four,471,888),
	TESTCASE(day_five_p1_a,"CMZ"),
	TESTCASE(day_five_p2_a,"MCD"),
	DAY(five,"TPGVQPFDH","DMRDFRHHH"),
	TESTCASE(day_six_p1_testcase<0>,7),
	TESTCASE(day_six_p1_testcase<1>,5),
	TESTCASE(day_six_p1_testcase<2>,6),
	TESTCASE(day_six_p1_testcase<3>,10),
	TESTCASE(day_six_p1_testcase<4>,11),
	TESTCASE(day_six_p2_testcase<0>,19),
	TESTCASE(day_six_p2_testcase<1>,23),
	TESTCASE(day_six_p2_testcase<2>,23),
	TESTCASE(day_six_p2_testcase<3>,29),
	TESTCASE(day_six_p2_testcase<4>,26),
	DAY(six,1623,3774),
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