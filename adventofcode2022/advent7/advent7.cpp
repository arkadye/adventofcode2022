#include "advent7.h"
#include "../advent/advent_utils.h"

#define ENABLE_DAY7DBG 1
#ifdef NDEBUG
#define DAY7DBG 0
#else
#define DAY7DBG ENABLE_DAY7DBG
#endif

#if DAY7DBG
	#include <iostream>
#endif

namespace
{
#if DAY7DBG
	std::ostream & log = std::cout;
#else
	struct {	template <typename T> auto& operator<<(const T&) const noexcept { return *this; } } log;
#endif
}

#include <string>
#include <variant>
#include <vector>
#include <memory>
#include <algorithm>

#include "istream_line_iterator.h"
#include "split_string.h"
#include "trim_string.h"
#include "parse_utils.h"
#include "to_value.h"
#include "int_range.h"

namespace
{
	using FileSize = int64_t;
	constexpr std::string_view root_folder = "/";
	constexpr std::string_view user_cmd_prefix = "$";
	constexpr std::string_view ls_cmd = "ls";
	constexpr std::string_view cd_cmd = "cd";
	constexpr std::string_view folder_up_cmd = "..";
	constexpr std::string_view dir_prefix = "dir";
	constexpr FileSize p1_threshold = 100'000;
	constexpr FileSize total_space = 70'000'000;
	constexpr FileSize required_space = 30'000'000;

	struct File
	{
		std::string name;
		FileSize size = -1;
	};

	class Directory
	{
	private:
		mutable FileSize total_file_size = -1;
		mutable FileSize total_subfolder_size = -1;
		void reset_total_subfolder_size()
		{
			total_subfolder_size = -1;
			if (parent != nullptr)
			{
				parent->reset_total_subfolder_size();
			}
		}
		void reset_total_file_size()
		{
			total_file_size = -1;
			if (parent != nullptr)
			{
				parent->reset_total_subfolder_size();
			}
		}
		std::vector<File> files;
		std::vector<std::unique_ptr<Directory>> sub_directories;
		Directory* parent = nullptr;
		Directory* find_directory(std::string_view name) const
		{
			auto find_result = std::find_if(begin(sub_directories), end(sub_directories),
				[name](const std::unique_ptr<Directory>& dir)
				{
					AdventCheck(dir.get() != nullptr);
					return dir->name == name;
				});
			if (find_result == end(sub_directories)) return nullptr;
			const std::unique_ptr<Directory>& result = *find_result;
			return result.get();
		}
	public:
		std::string name;
		Directory* get_parent() const { return parent; }
		const std::vector<File>& get_files() const { return files; }
		const std::vector<std::unique_ptr<Directory>>& get_subdirectories() const { return sub_directories;  }
		Directory* add_directory(std::string_view name)
		{
			AdventCheck(find_directory(name) == nullptr);
			auto new_dir = std::make_unique<Directory>();
			Directory* result = new_dir.get();
			AdventCheck(result != nullptr);
			new_dir->name = name;
			new_dir->parent = this;
			sub_directories.push_back(std::move(new_dir));
			return result;
		}
		Directory* get_subdirectory(std::string_view name)
		{
			Directory* result = find_directory(name);
			return (result != nullptr) ? result : add_directory(name);
		}

		const File& add_file(std::string_view filename, int size)
		{
			File new_file;
			new_file.name = filename;
			new_file.size = size;
			files.push_back(std::move(new_file));
			return files.back();
		}

		FileSize get_size(bool recursive) const
		{
			if (total_file_size < 0)
			{
				total_file_size = std::transform_reduce(begin(files), end(files), FileSize{ 0 }, std::plus<FileSize>{},
					[](const File& f) { return f.size; });
			}
			if (recursive && total_subfolder_size < 0)
			{
				total_subfolder_size = std::transform_reduce(begin(sub_directories), end(sub_directories), FileSize{ 0 }, std::plus<FileSize>{},
					[](const std::unique_ptr<Directory>& dir_ptr)
					{
						return dir_ptr->get_size(true);
					});
			}
			return total_file_size + (recursive ? total_subfolder_size : 0);
		}
	};

	enum class CommandType
	{
		cd,
		ls,
		invalid
	};

	struct Command
	{
		CommandType command_type = CommandType::invalid;
		std::string_view arg;
	};

	Command read_as_command(std::string_view line)
	{
		using utils::trim_left;
		using utils::trim_string;
		using utils::remove_specific_prefix;
		Command result;

		if (line.starts_with(user_cmd_prefix))
		{
			const std::string_view command = trim_left(remove_specific_prefix(line, user_cmd_prefix));

			if (command.starts_with(ls_cmd))
			{
				result.command_type = CommandType::ls;
			}
			else if (command.starts_with(cd_cmd))
			{
				result.command_type = CommandType::cd;
				result.arg = trim_string(remove_specific_prefix(command, cd_cmd));
			}
		}
		return result;
	}

	Directory get_directory_structure(std::istream& input)
	{
		Directory result;
		result.name = std::string{ root_folder };
		Directory* working_directory = nullptr;
		bool expects_user_input = true;

		for (const std::string& line : utils::istream_line_range{input})
		{
			const Command command = read_as_command(line);

			switch (command.command_type)
			{
			case CommandType::cd:
				AdventCheck(!command.arg.empty());
				if (command.arg == root_folder)
				{
					working_directory = &result;
				}
				else
				{
					AdventCheck(working_directory != nullptr);
					if (command.arg == folder_up_cmd)
					{
						AdventCheckMsg(working_directory->get_parent() != nullptr, "Can only go up a directory if there is a directory to go to.");
						working_directory = working_directory->get_parent();
					}
					else
					{
						working_directory = working_directory->get_subdirectory(command.arg);
					}
				}
				expects_user_input = true;
				break;
			case CommandType::ls:
				expects_user_input = false;
				break;
			case CommandType::invalid:
				AdventCheck(!expects_user_input);
				if (line.starts_with(dir_prefix))
				{
					const auto dir_name = utils::trim_left(utils::remove_specific_prefix(line, dir_prefix));
					working_directory->add_directory(dir_name);
				}
				else
				{
					auto [file_size_str, file_name] = utils::split_string_at_first(line, ' ');
					file_size_str = utils::trim_right(file_size_str);
					file_name = utils::trim_left(file_name);
					const int file_size = utils::to_value<int>(file_size_str);
					working_directory->add_file(file_name, file_size);
				}
				break;
			}
		}
		return result;
	}

	void print_directory(std::ostream& output, const Directory& dir, int depth = 0)
	{
		auto add_prefix = [&output, depth]()
		{
			for (int i : utils::int_range{ depth })
			{
				output << "  ";
			}
			output << "- ";
		};
		output << dir.name << " (dir)\n";
		for (const File& f : dir.get_files())
		{
			add_prefix();
			output << f.name << " (file size=" << f.size << ")\n";
		}
		for (const std::unique_ptr<Directory>& dir_ptr : dir.get_subdirectories())
		{
			add_prefix();
			print_directory(output, *dir_ptr, depth + 1);
			output << '\n';
		}
		if (dir.get_files().empty() && dir.get_subdirectories().empty())
		{
			add_prefix();
			output << "EMPTY\n";
		}
	}

	FileSize solve_p1_generic(const Directory& dir, FileSize threshold)
	{
		const auto& subdirs = dir.get_subdirectories();
		const FileSize subdir_result = std::transform_reduce(begin(subdirs), end(subdirs), FileSize{ 0 }, std::plus<FileSize>{},
			[threshold](const std::unique_ptr<Directory>& subdir)
			{
				return solve_p1_generic(*subdir, threshold);
			});
		const FileSize folder_size = dir.get_size(true);
		return subdir_result + (folder_size <= threshold ? folder_size : FileSize{ 0 });
	}

	FileSize solve_p1(std::istream& input)
	{
		const Directory dir = get_directory_structure(input);
		return solve_p1_generic(dir, p1_threshold);
	}
}

namespace
{
	FileSize get_size_of_smallest_folder_at_least(const Directory& dir, FileSize space_to_free)
	{
		FileSize result = -1;
		auto check_and_set_result = [&result, space_to_free](FileSize candidate)
		{
			if (candidate < space_to_free)
			{
				return;
			}
			if (result < 0)
			{
				result = candidate;
				return;
			}
			if (candidate < result)
			{
				result = candidate;
				return;
			}
			return;
		};

		for (const std::unique_ptr<Directory>& subdir : dir.get_subdirectories())
		{
			const FileSize sub_result = get_size_of_smallest_folder_at_least(*subdir, space_to_free);
			check_and_set_result(sub_result);
		}
		const FileSize my_size = dir.get_size(true);
		check_and_set_result(my_size);
		return result;
	}

	FileSize solve_p2(const Directory& dir, FileSize total_space, FileSize required_space)
	{
		const FileSize used_space = dir.get_size(true);
		const FileSize free_space = total_space - used_space;
		const FileSize min_free_size = required_space - free_space;
		AdventCheck(min_free_size > 0);
		return get_size_of_smallest_folder_at_least(dir, min_free_size);
	}

	FileSize solve_p2(std::istream& input)
	{
		const Directory dir = get_directory_structure(input);
		return solve_p2(dir, total_space, required_space);
	}
}

namespace
{
	std::istringstream testcase_a()
	{
		return std::istringstream{
			"$ cd /\n"
			"$ ls\n"
			"dir a\n"
			"14848514 b.txt\n"
			"8504156 c.dat\n"
			"dir d\n"
			"$ cd a\n"
			"$ ls\n"
			"dir e\n"
			"29116 f\n"
			"2557 g\n"
			"62596 h.lst\n"
			"$ cd e\n"
			"$ ls\n"
			"584 i\n"
			"$ cd ..\n"
			"$ cd ..\n"
			"$ cd d\n"
			"$ ls\n"
			"4060174 j\n"
			"8033020 d.log\n"
			"5626152 d.ext\n"
			"7214296 k"
		};
	}
}

ResultType day_seven_p1_a()
{
	auto input = testcase_a();
	return solve_p1(input);
}

ResultType day_seven_p2_a()
{
	auto input = testcase_a();
	return solve_p2(input);
}

ResultType advent_seven_p1()
{
	auto input = advent::open_puzzle_input(7);
	return solve_p1(input);
}

ResultType advent_seven_p2()
{
	auto input = advent::open_puzzle_input(7);
	return solve_p2(input);
}

#undef DAY7DBG
#undef ENABLE_DAY7DBG