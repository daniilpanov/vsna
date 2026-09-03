#pragma once
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#include "client.h"
#include "config.h"

struct CommandInfo
{
	std::string name;
	std::string description;
	std::string usage{ "" };
};

class MenuItem {
  protected:
	Client& _client;

  public:
	const CommandInfo _info;

	virtual ~MenuItem() = default;
	MenuItem(Client& client, const CommandInfo& info) : _client(client), _info(info)
	{}
	virtual bool handle(const std::vector<std::string>&) = 0;
};

class ConnectCommand : public MenuItem {
  public:
	ConnectCommand(Client& client, const CommandInfo& info) : MenuItem(client, info)
	{}
	bool handle(const std::vector<std::string>& args) override
	{
		_client.connect(args);
		return false;
	};
};

class MyPathCommand : public MenuItem {
  public:
	MyPathCommand(Client& client, const CommandInfo& info) : MenuItem(client, info)
	{}
	bool handle(const std::vector<std::string>& args) override
	{
		_client.myPath(args);
		return false;
	};
};

class PrintCommand : public MenuItem {
  public:
	PrintCommand(Client& client, const CommandInfo& info) : MenuItem(client, info)
	{}
	bool handle(const std::vector<std::string>& args) override
	{
		_client.print();
		return false;
	};
};

class ExitCommand : public MenuItem {
  public:
	ExitCommand(Client& client, const CommandInfo& info) : MenuItem(client, info)
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
	HelpCommand(Client& client, const CommandInfo& info, CommandManager& manager)
	    : MenuItem(client, info), _manager(manager)
	{}
	bool handle(const std::vector<std::string>& args) override;
};