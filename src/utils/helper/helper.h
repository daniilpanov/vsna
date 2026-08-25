#pragma once
#include <boost/asio/ip/address.hpp>
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

inline STRING_VECTOR split(STRING_ARG input)
{
	STRING_VECTOR args;
	std::stringstream ss(input);
	std::string token;
	while (ss >> token)
	{
		args.push_back(trim(token));
	}
	return args;
}

inline bool isValidIPv4(STRING_ARG ipString)
{
	try
	{
		boost::asio::ip::address addr = boost::asio::ip::make_address(ipString);
		return addr.is_v4();
	}
	catch (const boost::system::system_error&)
	{
		return false;
	}
}

inline std::string join(ARG_VECTOR strings, STRING_ARG delimiter) {
    if (strings.empty()) return "";
    
    std::string result = strings[0];
    for (size_t i = 1; i < strings.size(); ++i) {
        result += delimiter + strings[i];
    }
    return result;
}