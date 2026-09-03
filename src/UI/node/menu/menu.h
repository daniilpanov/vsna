#pragma once
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#include "config.h"
#include "node.h"

struct CommandInfo
{
	std::string name;
	std::string description;
	std::string usage{ "" };
};

class MenuItem {
  protected:
	Node& _node;

  public:
	const CommandInfo _info;

	virtual ~MenuItem() = default;
	MenuItem(Node& node, const CommandInfo& info) : _node(node), _info(info)
	{}
	virtual bool handle(const std::vector<std::string>&) = 0;
};

class ConnectCommand : public MenuItem {
  public:
	ConnectCommand(Node& node, const CommandInfo& info) : MenuItem(node, info)
	{}
	bool handle(const std::vector<std::string>& args) override
	{
		if (args.size() != 1)
		{
			std::cerr << "[!] Usage: " << _info.usage << std::endl;
			return false;
		}
		auto parts = split(args[0], ":");
		if (parts.size() != 2)
		{
			std::cerr << "[!] Usage: " << _info.usage << std::endl;
			return false;
		}
		_node.connect(parts[0], parts[1]);
		return false;
	};
};

class MyPathCommand : public MenuItem {
  public:
	MyPathCommand(Node& node, const CommandInfo& info) : MenuItem(node, info)
	{}
	bool handle(const std::vector<std::string>& args) override
	{
		_node.myPath();
		return false;
	};
};

class PrintCommand : public MenuItem {
  public:
	PrintCommand(Node& node, const CommandInfo& info) : MenuItem(node, info)
	{}
	bool handle(const std::vector<std::string>& args) override
	{
		_node.print();
		return false;
	};
};

class ExitCommand : public MenuItem {
  public:
	ExitCommand(Node& node, const CommandInfo& info) : MenuItem(node, info)
	{}
	bool handle(const std::vector<std::string>& args) override
	{
		std::cout << "[~] Program was exit." << std::endl;
		return true;
	}
};

class CommandManager;

class HelpCommand : public MenuItem {
	CommandManager& _manager;

  public:
	HelpCommand(Node& node, const CommandInfo& info, CommandManager& manager)
	    : MenuItem(node, info), _manager(manager)
	{}
	bool handle(const std::vector<std::string>& args) override;
};
