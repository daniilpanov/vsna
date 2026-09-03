#include <gtest/gtest.h>

#include "helper.h"

TEST(Helper, TrimStripsWhitespace)
{
	EXPECT_EQ(trim("  hello \t"), "hello");
	EXPECT_EQ(trim("   "), "");
	EXPECT_EQ(trim(""), "");
	EXPECT_EQ(trim("no-trim"), "no-trim");
}

TEST(Helper, TrimStripsNewlines)
{
	EXPECT_EQ(trim("\nfoo\r\n"), "foo");
}

TEST(Helper, SplitDefaultDelimiter)
{
	auto parts = split("alpha beta gamma");
	ASSERT_EQ(parts.size(), 3u);
	EXPECT_EQ(parts[0], "alpha");
	EXPECT_EQ(parts[1], "beta");
	EXPECT_EQ(parts[2], "gamma");
}

TEST(Helper, SplitCustomDelimiter)
{
	auto parts = split("1.2.3", ".");
	ASSERT_EQ(parts.size(), 3u);
	EXPECT_EQ(parts[0], "1");
	EXPECT_EQ(parts[2], "3");
}

TEST(Helper, SplitEmptyInput)
{
	auto parts = split("");
	ASSERT_EQ(parts.size(), 1u);
	EXPECT_EQ(parts[0], "");
}

TEST(Helper, SplitEmptyDelimiterReturnsWholeInput)
{
	auto parts = split("abcd", "");
	ASSERT_EQ(parts.size(), 1u);
	EXPECT_EQ(parts[0], "abcd");
}

TEST(Helper, JoinBasic)
{
	std::vector<std::string> v{ "a", "b", "c" };
	EXPECT_EQ(join(v, ", "), "a, b, c");
}

TEST(Helper, JoinEmptyVector)
{
	std::vector<std::string> v;
	EXPECT_EQ(join(v), "");
}

TEST(Helper, JoinEmptyStringsWithCustomDelimiter)
{
	std::vector<std::string> v{ "x" };
	EXPECT_EQ(join(v, "::"), "x");
}

TEST(Helper, IsValidIPv4AcceptsLoopback)
{
	EXPECT_TRUE(isValidIPv4("127.0.0.1"));
	EXPECT_TRUE(isValidIPv4("0.0.0.0"));
	EXPECT_TRUE(isValidIPv4("192.168.1.1"));
}

TEST(Helper, IsValidIPv4RejectsV6)
{
	EXPECT_FALSE(isValidIPv4("::1"));
	EXPECT_FALSE(isValidIPv4("fe80::1"));
}

TEST(Helper, IsValidIPv4RejectsGarbage)
{
	EXPECT_FALSE(isValidIPv4(""));
	EXPECT_FALSE(isValidIPv4("not-an-ip"));
	EXPECT_FALSE(isValidIPv4("999.1.1.1"));
	EXPECT_FALSE(isValidIPv4("1.2.3"));
}
