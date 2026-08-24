#pragma once
#include <algorithm>
#include <sstream>
#include <string>

#include "types.h"

constexpr uint16_t max_length{ 1024 };
constexpr uint16_t max_threads{ 4 };

inline std::string trim(STRING_ARG s) {
  size_t begin = s.find_first_not_of(" \t\r\n");
  if (begin == std::string::npos) return "";
  size_t end = s.find_last_not_of(" \t\r\n");
  return s.substr(begin, end - begin + 1);
}

inline ARG_VECTOR splitArgs(STRING_ARG input)
{
	ARG_VECTOR args;
	std::stringstream ss(input);
	std::string token;
	while (ss >> token)
	{
		args.push_back(trim(token));
	}
	return args;
}
