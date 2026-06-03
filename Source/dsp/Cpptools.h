#pragma once
#include <stdio.h>
#include <string>

namespace DirtyTools
{
	template <typename... Args>
	std::string OldFormat(const char* fmt, Args... args) {
		int size = std::snprintf(nullptr, 0, fmt, args...);
		if (size < 0) {
			return "";
		}
		std::string result(size, '\0');
		std::snprintf(result.data(), result.size() + 1, fmt, args...);
		return result;
	}
}