#include <gtest/gtest.h>

#include <stdexcept>

#include "addr.h"

TEST(Addr, DefaultValues)
{
	Addr a;
	EXPECT_EQ(a.ip(), "0.0.0.0");
	EXPECT_EQ(a.port(), "5555");
}

TEST(Addr, ConstructorSetsValues)
{
	Addr a("127.0.0.1", "8080");
	EXPECT_EQ(a.ip(), "127.0.0.1");
	EXPECT_EQ(a.port(), "8080");
	EXPECT_EQ(a.portNum(), 8080);
}

TEST(Addr, ToString)
{
	Addr a("10.0.0.1", "1234");
	EXPECT_EQ(a.toString(), "10.0.0.1:1234");
}

TEST(Addr, InvalidIpThrows)
{
	EXPECT_THROW(Addr("999.1.1.1", "80"), std::invalid_argument);
	EXPECT_THROW(Addr("localhost", "80"), std::invalid_argument);
	EXPECT_THROW(Addr("", "80"), std::invalid_argument);
	EXPECT_THROW(Addr("::1", "80"), std::invalid_argument);
}

TEST(Addr, InvalidPortThrows)
{
	EXPECT_THROW(Addr("127.0.0.1", ""), std::invalid_argument);
	EXPECT_THROW(Addr("127.0.0.1", "abc"), std::invalid_argument);
	EXPECT_THROW(Addr("127.0.0.1", "-1"), std::out_of_range);
	EXPECT_THROW(Addr("127.0.0.1", "70000"), std::out_of_range);
}

TEST(Addr, SetIpAndPort)
{
	Addr a;
	a.setIp("8.8.8.8");
	a.setPort("53");
	EXPECT_EQ(a.ip(), "8.8.8.8");
	EXPECT_EQ(a.port(), "53");
}

TEST(Addr, PortNormalizedToString)
{
	// stoi strips leading zeros.
	Addr a("127.0.0.1", "007");
	EXPECT_EQ(a.port(), "7");
}
