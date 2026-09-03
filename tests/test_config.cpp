#include <gtest/gtest.h>

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <unistd.h>

#include "config.h"

namespace {

std::filesystem::path makeTempDir()
{
	return std::filesystem::temp_directory_path() / ("vsna-test-" + std::to_string(::getpid()));
}

class ConfigTest : public ::testing::Test {
  protected:
	void SetUp() override
	{
		dir = makeTempDir();
		std::filesystem::create_directories(dir);
	}

	void TearDown() override
	{
		std::error_code ec;
		std::filesystem::remove_all(dir, ec);
	}

	std::filesystem::path dir;
};

} // namespace

TEST_F(ConfigTest, DefaultValues)
{
	Config c;
	EXPECT_EQ(c.getAddr().ip(), "0.0.0.0");
	EXPECT_EQ(c.getAddr().port(), "5555");
}

TEST_F(ConfigTest, ConstructorSetsValues)
{
	Config c(Addr("127.0.0.1", "9000"), dir.string());
	EXPECT_EQ(c.getAddr().toString(), "127.0.0.1:9000");
	// setPath resolves to canonical path.
	EXPECT_EQ(c.getPath(), std::filesystem::canonical(dir).string());
}

TEST_F(ConfigTest, ToStringContainsConfig)
{
	Config c(Addr("127.0.0.1", "9000"), dir.string());
	std::string s = c.toString();
	EXPECT_NE(s.find("ADDR: 127.0.0.1:9000"), std::string::npos);
	EXPECT_NE(s.find("PATH:"), std::string::npos);
}

TEST_F(ConfigTest, EmptyPathThrows)
{
	Config c;
	EXPECT_THROW(c.setPath(""), std::invalid_argument);
}

TEST_F(ConfigTest, NonexistentPathThrows)
{
	Config c;
	EXPECT_THROW(c.setPath((dir / "missing").string()), std::runtime_error);
}

TEST_F(ConfigTest, PathToFileThrows)
{
	auto file = dir / "somefile.txt";
	{
		std::ofstream ofs(file.string());
		ofs << "x";
	}
	Config c;
	EXPECT_THROW(c.setPath(file.string()), std::runtime_error);
}

TEST_F(ConfigTest, LoadFromFileParsesJson)
{
	auto path = dir / "config.json";
	{
		std::ofstream ofs(path.string());
		ofs << "{\n"
		    << "  \"ip\": \"127.0.0.1\",\n"
		    << "  \"port\": \"8000\",\n"
		    << "  \"path\": \"" << dir.string() << "\"\n"
		    << "}\n";
	}
	Config c = Config::loadFromFile(path.string());
	EXPECT_EQ(c.getAddr().toString(), "127.0.0.1:8000");
	EXPECT_EQ(c.getPath(), std::filesystem::canonical(dir).string());
}

TEST_F(ConfigTest, LoadFromFileMissingThrows)
{
	EXPECT_THROW(Config::loadFromFile((dir / "nope.json").string()), std::runtime_error);
}

TEST_F(ConfigTest, LoadFromFileInvalidJsonThrows)
{
	auto path = dir / "bad.json";
	{
		std::ofstream ofs(path.string());
		ofs << "{ not valid json !!!\n";
	}
	EXPECT_THROW(Config::loadFromFile(path.string()), std::exception);
}
