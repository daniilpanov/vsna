#pragma once
#include <boost/asio/ip/address.hpp>
#include <algorithm>
#include <sstream>
#include <string_view>

constexpr uint16_t max_length{ 1024 };
constexpr uint16_t max_threads{ 4 };

inline std::string trim(const std::string& s)
{
	size_t begin = s.find_first_not_of(" \t\r\n");
	if (begin == std::string::npos)
		return "";
	size_t end = s.find_last_not_of(" \t\r\n");
	return s.substr(begin, end - begin + 1);
}

inline std::vector<std::string> split(std::string_view input, std::string_view delimiter = " ")
{
	std::vector<std::string> result;
	if (delimiter.empty())
	{
		result.emplace_back(input);
		return result;
	}

	size_t start = 0;
	size_t end = input.find(delimiter);

	while (end != std::string::npos)
	{
		result.emplace_back(input.substr(start, end - start));
		start = end + delimiter.length();
		end = input.find(delimiter, start);
	}

	result.emplace_back(input.substr(start));
	return result;
}

inline bool isValidIPv4(const std::string& ipString)
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

inline std::string join(const std::vector<std::string>& strings, std::string_view delimiter = " ")
{
	std::string result;

	if (strings.empty())
		return result;

	// Reserve the exact output size up front so append operations never
	// trigger a reallocation while building the joined string.
	std::size_t length = delimiter.length() * (strings.size() - 1);
	for (const auto& s : strings)
		length += s.size();
	result.reserve(length);

	bool first = true;
	for (const auto& s : strings)
	{
		if (!first)
			result += delimiter;
		result += s;
		first = false;
	}
	return result;
}
