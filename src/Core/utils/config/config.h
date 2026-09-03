#pragma once
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>

#include "addr.h"

using json = nlohmann::json;

class Config {
  public:
	Config() = default;
	Config(const Addr&, const std::string&);

	static Config loadFromFile(const std::string&);
	void setAddr(const Addr&);
	void setAddr(Addr&&);
	void setPath(const std::string&);

	Addr getAddr() const
	{
		return this->_addr;
	}
	std::string getPath() const
	{
		return this->_path;
	}

	std::string toString() const;

  private:
	Addr _addr;
	std::string _path;
};