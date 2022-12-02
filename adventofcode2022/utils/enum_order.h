#pragma once

namespace utils
{
	template <typename T>
	bool enum_order(T left, T right)
	{
		static_assert(std::is_enum_v<T>,"T must be an enum");
		using UT = std::underlying_type_t<T>;
		const auto left_val = static_cast<UT>(left);
		const auto right_val = static_cast<UT>(right);
		return left_val < right_val;
	}

	template <typename T>
	struct EnumSorter
	{
		constexpr bool operator()(T left, T right) const
		{
			return enum_order(left,right);
		}
	};
}