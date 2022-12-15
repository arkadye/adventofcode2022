#include "advent/advent_of_code.h"

#include <iostream>

int main(int argc, char** argv)
{
	// Use the filter to only run certain tests.
	// This uses some magic to test against the name of the function,
	// so putting "eighteen" as the argument will only run advent_eighteen_p1()
	// and advent_eighteen_p2() (as well as any other test functions with "eighteen"
	// in the function name.
	// Leave blank to run everything.
	if (argc > 1)
	{
		verify_all(argv[1]);
	}
	else
	{
		verify_all();
	}
	return 0;
}