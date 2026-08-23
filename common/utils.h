#pragma once
#include <algorithm>
#include <sstream>
#include <string>

#include "types.h"

constexpr uint16_t max_length{ 1024 };
constexpr uint16_t max_threads{ 4 };

inline ARG_VECTOR splitArgs(STRING_ARG input)
{
	ARG_VECTOR args;
	std::stringstream ss(input);
	std::string token;
	while (ss >> token)
	{
		args.push_back(token);
	}
	return args;
}