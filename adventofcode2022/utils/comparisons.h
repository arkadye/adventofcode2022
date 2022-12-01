#pragma once

namespace utils
{
	template <typename FwdIt, typename Transform>
	[[nodiscard]] inline FwdIt min_element_transform(FwdIt first, FwdIt last, const Transform& transform) noexcept
	{
		if (first == last)
		{
			return last;
		}
		using ValType = decltype(*first);
		FwdIt result = first;
		for (FwdIt it = first; it != last; ++it)
		{
			if (transform(*it) < transform(*result))
			{
				result = it;
			}
		}
		return result;
	}

	template <typename FwdIt, typename Transform>
	[[nodiscard]] inline FwdIt max_element_transform(FwdIt first, FwdIt last, const Transform& transform) noexcept
	{
		if (first == last)
		{
			return last;
		}
		using ValType = decltype(*first);
		FwdIt result = first;
		for (FwdIt it = first; it != last; std::advance(it,1))
		{
			if (transform(*it) > transform(*result))
			{
				result = it;
			}
		}
		return result;
	}

	template <typename FwdIt, typename Transform>
	[[nodiscard]] inline auto min_transform(FwdIt first, FwdIt last, const Transform& transform) noexcept
	{
		return *min_element_transform(first, last, transform);
	}

	template <typename FwdIt, typename Transform>
	[[nodiscard]] inline auto max_transform(FwdIt first, FwdIt last, const Transform& transform) noexcept
	{
		return *max_element_transform(first, last, transform);
	}

	template<typename T>
	struct Larger
	{
		constexpr T operator()(const T& x, const T& y) const noexcept
		{
			return std::max<T>(x,y);
		}
	};

	template<typename T>
	struct Smaller
	{
		constexpr T operator()(const T& x, const T& y) const noexcept
		{
			return std::min<T>(x, y);
		}
	};

	namespace utils_internal
	{
		template <typename BinaryOp, typename T>
		T arg_folder(const BinaryOp& op, const T& first_arg, const T& second_arg)
		{
			return op(first_arg,second_arg);
		}

		template <typename BinaryOp, typename T, typename...ExtraArgs>
		T arg_folder(const BinaryOp& op, const T& first_arg, const T& second_arg, const T& third_arg, const ExtraArgs&...extra_args)
		{
			return op(first_arg,arg_folder(op,second_arg,third_arg,extra_args...));
		}
	}

	template <typename...Args>
	auto min(const Args&... args) noexcept
	{
		static_assert(sizeof...(Args) >= 2,"utils::min must be called with at least two arguments");
	}

	template <typename T>
	T max(const T& FirstArg, const T& SecondArg)
	{
		return Larger{}(FirstArg, SecondArg);
	}

	template <typename T, typename...Args>
	T max(const T& FirstArg, const T& SecondArg, const T& ThirdArg, const Args&... RestOfArgs)
	{
		return Larger{}(FirstArg, max(SecondArg, ThirdArg, RestOfArgs...));
	}

	namespace ranges
	{
		template <typename RangeType, typename Transform>
		[[nodiscard]] inline auto min_element_transform(RangeType&& range, const Transform& transform) noexcept
		{
			return min_element_transform(range.begin(),range.end(),transform);
		}

		template <typename RangeType, typename Transform>
		[[nodiscard]] inline auto max_element_transform(RangeType&& range, const Transform& transform) noexcept
		{
			return max_element_transform(range.begin(), range.end(), transform);
		}

		template <typename RangeType, typename Transform>
		[[nodiscard]] inline auto min_transform(RangeType&& range, const Transform& transform) noexcept
		{
			return min_transform(range.begin(), range.end(), transform);
		}

		template <typename RangeType, typename Transform>
		[[nodiscard]] inline auto max_transform(RangeType&& range, const Transform& transform) noexcept
		{
			return max_transform(range.begin(), range.end(), transform);
		}
	}
}