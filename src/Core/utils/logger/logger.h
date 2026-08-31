#pragma once

/*
    usage examples:
        logger::debug("Your message here");
        logger::info("You can provide args like: {}, {}", arg1, arg2);
    all levels / functions: debug, info, warn, error
 */

#include <print>
#include <format>
#include <string_view>
#include <source_location>

namespace logger {
enum class Level { DEBUG, INFO, WARN, ERROR };

template <Level l, class... Args> struct print
{};

template <Level l, class... Args> struct print<l, const char *, Args...>
{
	print(const char *msg, Args&&...args,
	      std::source_location loc = std::source_location::current())
	{

		auto levelString = "DEBUG";

		if constexpr (l == Level::INFO)
		{
			levelString = "INFO";
		}
		else if constexpr (l == Level::WARN)
		{
			levelString = "WARN";
		}
		else if constexpr (l == Level::ERROR)
		{
			levelString = "ERROR";
		}
		std::println("[ {} ] {} : line {}  {}", levelString, loc.file_name(), loc.line(),
		             std::vformat(msg, std::make_format_args(args...)));
	}
};

// template deduction guide
template <Level l, class... Args> print(Args...) -> print<l, Args...>;

template <class... Args> using debug = print<Level::DEBUG, Args...>;

template <class... Args> using info = print<Level::INFO, Args...>;

template <class... Args> using warn = print<Level::WARN, Args...>;

template <class... Args> using error = print<Level::ERROR, Args...>;
}
