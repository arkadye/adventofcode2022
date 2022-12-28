#pragma once

#include "advent_testcase_setup.h"

static const std::string DEFAULT_FILTER = "";

static const std::string DAY_10_P2_B_RESULT =
R"(
##..##..##..##..##..##..##..##..##..##..
###...###...###...###...###...###...###.
####....####....####....####....####....
#####.....#####.....#####.....#####.....
######......######......######......####
#######.......#######.......#######.....)";

static const std::string ADVENT_10_RESULT =
R"(
###  #    #  # #    #  # ###  #### #  # 
#  # #    #  # #    # #  #  #    # #  # 
#  # #    #  # #    ##   ###    #  #### 
###  #    #  # #    # #  #  #  #   #  # 
#    #    #  # #    # #  #  # #    #  # 
#    ####  ##  #### #  # ###  #### #  # )";

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
	TESTCASE(day_seven_p1_a,95437),
	TESTCASE(day_seven_p2_a,24933642),
	DAY(seven,1908462,3979145),
	TESTCASE(day_eight_p1_a,21),
	TESTCASE(day_eight_p2_a,8),
	DAY(eight,1676,313200),
	TESTCASE(day_nine_p1_a,13),
	TESTCASE(day_nine_p2_a,1),
	TESTCASE(day_nine_p2_b,36),
	DAY(nine,5619,2376),
	TESTCASE(day_ten_p1_a, -1),
	TESTCASE(day_ten_p1_b, 13140),
	TESTCASE(day_ten_p2_b, DAY_10_P2_B_RESULT),
	DAY(ten,14240,ADVENT_10_RESULT),
	TESTCASE(day_eleven_p1_a,10605),
	TESTCASE(day_eleven_p2_a<1>,4*6),
	TESTCASE(day_eleven_p2_a<20>,103*99),
	TESTCASE(day_eleven_p2_a<1000>, 5204 * 5192),
	TESTCASE(day_eleven_p2_a<2000>, 10419 * 10391),
	TESTCASE(day_eleven_p2_a<3000>, 15638 * 15593),
	TESTCASE(day_eleven_p2_a<4000>, 20858 * 20797),
	TESTCASE(day_eleven_p2_a<5000>, 26075 * 26000),
	TESTCASE(day_eleven_p2_a<6000>, 31294 * 31204),
	TESTCASE(day_eleven_p2_a<7000>, 36508 * 36400),
	TESTCASE(day_eleven_p2_a<8000>, 41728 * 41606),
	TESTCASE(day_eleven_p2_a<9000>, int64_t{46945} * int64_t{46807}),
	TESTCASE(day_eleven_p2_a<10000>, 2713310158),
	DAY(eleven,62491,17408399184),
	TESTCASE(day_twelve_p1_a,31),
	TESTCASE(day_twelve_p2_a,29),
	DAY(twelve,394,388),
	TESTCASE(day_thirteen_p1_a,13),
	TESTCASE(day_thirteen_p2_a,140),
	DAY(thirteen,5393,26712),
	TESTCASE(day_fourteen_p1_a,24),
	TESTCASE(day_fourteen_p2_a,93),
	DAY(fourteen,888,26461),
	TESTCASE(day_fifteen_p1_a,26),
	TESTCASE(day_fifteen_p2_a,56000011),
	DAY(fifteen,4725496,12051287042458),
	TESTCASE(day_sixteen_p1_a,1651),
	TESTCASE(day_sixteen_p2_a,1707),
	DAY(sixteen,1871,2416),
	DAY(seventeen,Dummy{},Dummy{}),
	DAY(eighteen,Dummy{},Dummy{}),
	DAY(nineteen,Dummy{},Dummy{}),
	DAY(twenty,Dummy{},Dummy{}),
	TESTCASE(day_twentyone_p1_a,152),
	TESTCASE(day_twentyone_p2_a,301),
	DAY(twentyone,85616733059734,3560324848168),
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