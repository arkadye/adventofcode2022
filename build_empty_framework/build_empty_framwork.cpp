#include <fstream>
#include <array>
#include <string>
#include <filesystem>
#include <sstream>
#include <iostream>
#include <cassert>

constexpr int NUM_DAYS = 25;

const char* advent_testcase_setup_contents = R"(#pragma once

#include <functional>
#include <string>

using TestFunc = std::function<ResultType()>;

// This describes a test to run.
struct verification_test
{
	std::string name;
	TestFunc test_func;
	std::string expected_result;
	bool result_known;
};

// A type to use to indicate the result is not known yet. Using this in a verification test
// will run the test and report the result, but will count as neither pass nor failure.
struct Dummy {};

verification_test make_test(std::string name, TestFunc func, int64_t result);
verification_test make_test(std::string name, TestFunc func, std::string result);
verification_test make_test(std::string name, TestFunc func, Dummy);
verification_test make_test(std::string name, Dummy, Dummy);

#define ARG(func_name) std::string{ #func_name },func_name
#define TESTCASE(func_name,expected_result) make_test(ARG(func_name),expected_result)
#define FUNC_NAME(day_num,part_num) advent_ ## day_num ## _p ## part_num
#define TEST_DECL(day_num,part_num,expected_result) TESTCASE(FUNC_NAME(day_num,part_num),expected_result)
#define DAY(day_num,part1_result,part2_result) \
	TEST_DECL(day_num,1,part1_result), \
	TEST_DECL(day_num,2,part2_result))";

const char* advent_of_code_header_contents = R"(#pragma once

#include <string>

bool verify_all(const std::string& filter);
bool verify_all();)";

const char* advent_types_header_contents = R"(#pragma once

#include <string>
#include <variant>
#include <cstdint>

using ResultType = std::variant<std::string, int64_t>;)";

const char* advent_of_code_testcases_source_contents = R"(#include <functional>
#include <string>
#include <iostream>
#include <array>
#include <algorithm>
#include <sstream>
#include <chrono>
#include <optional>
#include <iomanip>
#include <cassert>
#include <numeric>

#include "../advent/advent_of_code.h"
#include "../advent/advent_headers.h"
#include "../advent/advent_setup.h"

std::string to_string(const ResultType& rt)
{
	if (std::holds_alternative<std::string>(rt)) return std::get<std::string>(rt);
	else if (std::holds_alternative<int64_t>(rt)) return std::to_string(std::get<int64_t>(rt));
	assert(false);
	return "!ERROR!";
}

// Result a test can give.
enum class test_status : char
{
	pass,
	fail,
	unknown,
	filtered
};

// Full results of a test.
struct test_result
{
	std::string name;
	std::string result;
	std::string expected;
	test_status status = test_status::unknown;
	std::chrono::nanoseconds time_taken;
};

template <test_status status>
bool check_result(const test_result& result)
{
	return status == result.status;
}

std::string two_digits(int num)
{
	std::ostringstream oss;
	oss << std::setfill('0') << std::setw(2) << num;
	return oss.str();
}

std::optional<std::string> to_human_readable(long long count, int inner_max, int outer_max, const std::string& suffix)
{
	if (count < inner_max)
		return std::to_string(count) + suffix;

	if (count < (static_cast<decltype(count)>(inner_max) * static_cast<decltype(count)>(outer_max)))
	{
		std::ostringstream oss;
		oss << count / inner_max << ':' << two_digits(count % inner_max) << suffix;
		return oss.str();
	}

	return std::optional<std::string>{};
}

std::string to_human_readable(std::chrono::hours time)
{
	const auto count = time.count();
	std::ostringstream oss;
	oss << count / 24 << " days and " << count % 24 << " hours";
	return oss.str();
}

std::string to_human_readable(std::chrono::minutes time)
{
	const auto res = to_human_readable(time.count(), 60, 24, "s");
	if (res.has_value())
		return res.value();

	const auto h = std::chrono::duration_cast<std::chrono::hours>(time);
	return to_human_readable(h);
}

std::string to_human_readable(std::chrono::seconds time)
{
	const auto res = to_human_readable(time.count(), 60,60,"s");
	if (res.has_value())
		return res.value();

	const auto m = std::chrono::duration_cast<std::chrono::minutes>(time);
	return to_human_readable(m);
}

std::optional<std::string> to_human_readable(long long count, const std::string& suffix_short, const std::string& suffix_long)
{
	if (count < 10'000)
		return std::to_string(count) + suffix_short;
	if (count < 100'000)
	{
		std::ostringstream oss;
		oss << std::setprecision(3) << static_cast<double>(count) / 1000.0 << suffix_long;
		return oss.str();
	}
	return std::optional<std::string>{};
}

std::string to_human_readable(std::chrono::milliseconds time)
{
	const auto res = to_human_readable(time.count(), "ms", "s");
	if (res.has_value())
		return res.value();

	const auto s = std::chrono::duration_cast<std::chrono::seconds>(time);
	return to_human_readable(s);
}

std::string to_human_readable(std::chrono::microseconds time)
{
	const auto res = to_human_readable(time.count(), "us", "ms");
	if (res.has_value())
		return res.value();

	const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(time);
	return to_human_readable(ms);
}

std::string to_human_readable(std::chrono::nanoseconds time)
{
	const auto res = to_human_readable(time.count(), "ns", "us");
	if (res.has_value())
		return res.value();

	const auto us = std::chrono::duration_cast<std::chrono::microseconds>(time);
	return to_human_readable(us);
}

test_result run_test(const verification_test& test, std::string_view filter)
{
	if (test.name.find(filter) == test.name.npos)
	{
		return test_result{
			test.name,
			"",
			to_string(test.expected_result),
			test_status::filtered
		};
	}
	std::cout << "Running test " << test.name << ": ";
	const auto start_time = std::chrono::high_resolution_clock::now();
	const auto res = test.test_func();
	const auto end_time = std::chrono::high_resolution_clock::now();
	const std::chrono::nanoseconds time_taken = end_time - start_time;
	const auto string_result = to_string(res);
	std::cout << "took " << to_human_readable(time_taken) <<  " and got " << string_result << '\n';
	auto get_result = [&](test_status status)
	{
		return test_result{ test.name,string_result,test.expected_result,status,time_taken };
	};
	if (test.result_known && string_result == test.expected_result)
	{
		return get_result(test_status::pass);
	}
	else if (test.result_known && string_result != test.expected_result)
	{
		return get_result(test_status::fail);
	}
	return get_result(test_status::unknown);
}

bool verify_all(const std::string& filter)
{
	constexpr int NUM_TESTS = sizeof(tests) / sizeof(verification_test);
	std::array<test_result, NUM_TESTS> results;
	std::transform(tests, tests + NUM_TESTS, begin(results),
		std::bind(run_test,std::placeholders::_1,filter));
	auto result_to_string = [&filter](const test_result& result)
	{
		std::ostringstream oss;
		oss << result.name << ": " << to_string(result.result) << " - ";
		switch (result.status)
		{
		case test_status::pass:
			oss << "PASS\n";
			break;
		case test_status::fail:
			oss << "FAIL (expected " << to_string(result.expected) << ")\n";
			break;
		case test_status::filtered:
			return std::string{ "" };
		default: // unknown
			oss << "[Unknown]\n";
			break;
		}
		return oss.str();
	};

	std::transform(begin(results),end(results),std::ostream_iterator<std::string>(std::cout), result_to_string);

	auto get_count = [&results](auto pred)
	{
		return std::count_if(begin(results), end(results), pred);
	};

	const auto total_time = std::transform_reduce(begin(results), end(results), std::chrono::nanoseconds{ 0 },
		std::plus<std::chrono::nanoseconds>{}, [](const test_result& result) {return result.time_taken; });

	std::cout << 
		"RESULTS:\n"
		"    PASSED : " << get_count(check_result<test_status::pass>) << "\n"
		"    FAILED : " << get_count(check_result<test_status::fail>) << "\n"
		"    UNKNOWN: " << get_count(check_result<test_status::unknown>) << "\n"
		"    TIME   : " << to_human_readable(total_time) << '\n';
	return std::none_of(begin(results), end(results),check_result<test_status::fail>);
}

bool verify_all()
{
	return verify_all(DEFAULT_FILTER);
}

verification_test make_test(std::string name, TestFunc func, int64_t result)
{
	return make_test(std::move(name), func, std::to_string(result));
}

verification_test make_test(std::string name, TestFunc func, std::string result)
{
	return verification_test{ std::move(name),func,result,true };
}

verification_test make_test(std::string name, TestFunc func, Dummy)
{
	return verification_test{ std::move(name),func,"",false };
}

verification_test make_test(std::string name, Dummy, Dummy)
{
	return make_test(std::move(name), []() { std::cout << "Test not implemented yet"; return ResultType{ 0 }; }, Dummy{});
})";

const char* advent_utils_header_contents = R"(#pragma once

#include <fstream>
#include <sstream>
#include <cassert>
#include <string>
#include <string_view>

namespace advent
{

	inline std::ifstream open_puzzle_input(int day)
	{
		std::ostringstream name;
		name << "advent" << day << "/advent" << day << ".txt";
		auto result = std::ifstream{ name.str() };
		assert(result.is_open());
		return result;
	}

	inline std::ifstream open_testcase_input(int day, char id)
	{
		std::ostringstream name;
		name << "advent" << day << "/testcase_" << id << ".txt";
		auto result = std::ifstream{ name.str() };
		assert(result.is_open());
		return result;
	}

	class test_failed
	{
		std::string m_what;
	public:
		std::string_view what() const { return m_what; }
		explicit test_failed(std::string what_happened) : m_what{ std::move(what_happened) } {}
	};

	namespace
	{
		inline std::string make_what_happened_str_impl(std::ostringstream& msg)
		{
			return msg.str();
		}

		template <typename T, typename...Args>
		inline std::string make_what_happened_str_impl(std::ostringstream& msg, const T& first, const Args&...rest)
		{
			msg << ' ' << first;
			return make_what_happened_str_impl(msg, rest...);
		}

		template <typename T, typename...Args>
		inline std::string make_what_happened_str(const T& first, const Args& ... args)
		{
			std::ostringstream msg;
			msg << first;
			return make_what_happened_str_impl(msg, args...);
		}

		template <typename...Args>
		inline void check_advent_assert_impl(
			std::string_view file, 
			int line_no, 
			bool check_passes, 
			std::string_view check_str, 
			const Args&...msg)
		{
			if (!check_passes)
			{
				const auto file_break = file.find_last_of("\\/");
				if (file_break < file.size())
				{
					file.remove_prefix(file_break + 1);
				}
				auto what = make_what_happened_str(
					file,
					'(',
					line_no,
					"): '",
					check_str,
					'\'',
					msg...
				);

				test_failed error{ std::move(what) };
				throw error;
			}
		}

		inline void check_advent_assert(std::string_view file, int line_no, bool check_passes, std::string_view check_str)
		{
			check_advent_assert_impl(file, line_no, check_passes, check_str);
		}

		template <typename...Args>
		inline void check_advent_assert_msg(std::string_view file, int line_no, bool check_passes, std::string_view check_str, const Args&...args)
		{
			check_advent_assert_impl(file, line_no, check_passes, check_str," Msg: " , args...);
		}
	}
}

#ifdef _MSC_VER
#define InternalAdventPlatformSpecificUnreachable __assume(false)
#endif

#define AdventCheck(test_bool) advent::check_advent_assert(__FILE__,__LINE__,test_bool,#test_bool)
#define AdventCheckMsg(test_bool,...) advent::check_advent_assert_msg(__FILE__,__LINE__,test_bool,#test_bool,__VA_ARGS__)
#define AdventUnreachable() advent::check_advent_assert_msg(__FILE__,__LINE__,false,"Entered unreachable location!"); \
	InternalAdventPlatformSpecificUnreachable)";

const char* main_contents = R"(#include "advent/advent_of_code.h"

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
#ifndef WIN32
	std::cout << "Program finished. Press any key to continue.";
	std::cin.get();
#endif
	return 0;
})";

std::ofstream get_file(const std::filesystem::path& file)
{
	using namespace std::filesystem;
	if (exists(file))
	{
		auto new_filename = file;
		new_filename += ".bak";
		rename(file, new_filename);
	}
	assert(!exists(file));
	const auto folder = file.parent_path();
	std::filesystem::create_directories(folder);
	auto result = std::ofstream{ file };
	assert(result.is_open());
	return result;
}

void make_files_for_day(const std::filesystem::path& base, std::string_view day_name, int day_val)
{
	auto get_func_name = [&day_name](int part)
	{
		assert(part == 1 || part == 2);
		std::ostringstream oss;
		oss << "ResultType advent_" << day_name << "_p" << part << "()";
		return oss.str();
	};

	const std::string name_base = "advent" + std::to_string(day_val);

	{
		auto header = base / name_base / (name_base + ".h");
		auto header_file = get_file(header);
		header_file << "#pragma once\n\n"
			"#include \"../advent/advent_types.h\"\n\n"
			<< get_func_name(1) << ";\n"
			<< get_func_name(2) <<";";
	}

	{
		auto build_func = [&day_name, day_val, &get_func_name](int part_num)
		{
			assert(part_num == 1 || part_num == 2);
			std::ostringstream oss;
			oss << get_func_name(part_num) << "\n"
				"{\n"
				"\tauto input = advent::open_puzzle_input(" << day_val << ");\n";
			if (part_num == 2 && day_val == NUM_DAYS)
			{
				oss << "\treturn \"MERRY CHRISTMAS!\";\n";
			}
			else
			{
				oss << "\treturn solve_p" << part_num << "(input);\n";
			}
			oss << "}";
			return oss.str();
		};

		auto source = base / name_base / (name_base + ".cpp");
		auto source_file = get_file(source);
		const std::string debug_macro_name = "DAY" + std::to_string(day_val) + "DBG";
		source_file << "#include \"advent" << day_val << ".h\"\n"
			"#include \"../advent/advent_utils.h\"\n\n"
			"#define ENABLE_" << debug_macro_name << " 1\n"
			"#ifdef NDEBUG\n"
			"#define " << debug_macro_name << " 0\n"
			"#else\n"
			"#define " << debug_macro_name << ' ' << "ENABLE_" << debug_macro_name << "\n"
			"#endif\n\n"
			"#if " << debug_macro_name << "\n"
			"\t#include <iostream>\n"
			"#endif\n\n"
			"namespace\n"
			"{\n"
			"#if " << debug_macro_name << "\n"
			"\tstd::ostream & log = std::cout;\n"
			"#else\n"
			"\tstruct {	template <typename T> auto& operator<<(const T&) const noexcept { return *this; } } log;\n"
			"#endif\n"
			"}\n\n"
			"namespace\n"
			"{\n"
			"\tint solve_p1(std::istream& input)\n"
			"\t{\n"
			"\t\treturn 0;\n"
			"\t}\n"
			"}\n\n"
			"namespace\n"
			"{\n";
		if (day_val != NUM_DAYS)
		{
			source_file << "\tint solve_p2(std::istream& input)\n"
				"\t{\n"
				"\t\treturn 0;\n"
				"\t}\n";
		}
		source_file << "}\n\n"
			<< build_func(1) << "\n\n"
			<< build_func(2) << "\n\n"
			"#undef " << debug_macro_name << "\n"
			"#undef ENABLE_" << debug_macro_name;
	}

	{
		auto input = base / name_base / (name_base + ".txt");
		auto input_file = get_file(input);
	}
}

int main()
{
	const std::array<std::string, NUM_DAYS> names
	{
		"one",
		"two",
		"three",
		"four",
		"five",
		"six",
		"seven",
		"eight",
		"nine",
		"ten",
		"eleven",
		"twelve",
		"thirteen",
		"fourteen",
		"fifteen",
		"sixteen",
		"seventeen",
		"eighteen",
		"nineteen",
		"twenty",
		"twentyone",
		"twentytwo",
		"twentythree",
		"twentyfour",
		"twentyfive"
	};

	std::filesystem::path base;
	std::cout << "Put base path here:";
	std::cin >> base;
	auto setup_file = get_file(base / "advent" / "advent_setup.h");
	setup_file << "#pragma once\n\n"
		"#include \"advent_testcase_setup.h\"\n\n"
		"static const std::string DEFAULT_FILTER = \"\";\n\n"
		"static const verification_test tests[] =\n"
		"{\n";

	auto headers_file = get_file(base / "advent" / "advent_headers.h");
	headers_file << "#pragma once\n";

	for (int i = 0; i < NUM_DAYS; ++i)
	{
		const int day_num = i + 1;
		const std::string_view day_name = names[i];
		setup_file << "\tDAY(" << day_name << ",Dummy{},"
			<< (day_num == NUM_DAYS ? "\"MERRY CHRISTMAS!\"" : "Dummy{}")
			<< ')';
		if (day_num != NUM_DAYS)
		{
			setup_file << ',';
		}
		setup_file << '\n';
		make_files_for_day(base, day_name, day_num);

		headers_file << "\n#include \"../advent" << day_num << "/advent" << day_num << ".h\"";
	}

	setup_file << "};\n\n"
		"#undef ARG\n"
		"#undef TESTCASE\n"
		"#undef FUNC_NAME\n"
		"#undef TEST_DECL\n"
		"#undef DAY\n"
		"#undef DUMMY\n"
		"#undef DUMMY_DAY";

	{
		auto testcase_setup_file = get_file(base / "advent" / "advent_testcase_setup.h");
		testcase_setup_file << advent_testcase_setup_contents;
	}

	{
		auto advent_of_code_header = get_file(base / "advent" / "advent_of_code.h");
		advent_of_code_header << advent_of_code_header_contents;
	}

	{
		auto advent_types_header = get_file(base / "advent" / "advent_types.h");
		advent_types_header << advent_types_header_contents;
	}

	{
		auto advent_of_code_source = get_file(base / "src" / "advent_of_code_testcases.cpp");
		advent_of_code_source << advent_of_code_testcases_source_contents;
	}
	
	{
		auto advent_utils_header = get_file(base / "advent" / "advent_utils.h");
		advent_utils_header << advent_utils_header_contents;
	}

	{
		auto main_source = get_file(base / "main.cpp");
		main_source << main_contents;
	}
}