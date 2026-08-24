#pragma once
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#include "client.h"
#include "config.h"
#include "types.h"

struct CommandInfo {
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
	MenuItem(Client& client, const CommandInfo& info) 
	    : _client(client), _info(info)
	{}
	virtual bool handle(ARG_VECTOR) = 0;
};

class ConnectCommand : public MenuItem {
  public:
	ConnectCommand(Client& client, const CommandInfo& info) 
	    : MenuItem(client, info)
	{}
	bool handle(ARG_VECTOR args) override
	{
		_client.connect(args);
		return false;
	};
};

class ShowPathCommand : public MenuItem {
  public:
	ShowPathCommand(Client& client, const CommandInfo& info)
	    : MenuItem(client, info)
	{}
	bool handle(ARG_VECTOR args) override
	{
		_client.showPath(args);
		return false;
	};
};

class MyPathCommand : public MenuItem {
  public:
	MyPathCommand(Client& client, const CommandInfo& info)
	    : MenuItem(client, info)
	{}
	bool handle(ARG_VECTOR args) override
	{
		_client.myPath(args);
		return false;
	};
};

class SendFilesCommand : public MenuItem {
  public:
	SendFilesCommand(Client& client, const CommandInfo& info)
	    : MenuItem(client, info)
	{}
	bool handle(ARG_VECTOR args) override
	{
		_client.sendFiles(args);
		return false;
	};
};

class DownloadCommand : public MenuItem {
  public:
	DownloadCommand(Client& client, const CommandInfo& info)
	    : MenuItem(client, info)
	{}
	bool handle(ARG_VECTOR args) override
	{
		_client.download(args);
		return false;
	};
};

class PrintCommand : public MenuItem {
  public:
	PrintCommand(Client& client, const CommandInfo& info)
	    : MenuItem(client, info)
	{}
	bool handle(ARG_VECTOR args) override
	{
		_client.print();
		return false;
	};
};

class ExitCommand : public MenuItem {
  public:
	ExitCommand(Client& client, const CommandInfo& info)
	    : MenuItem(client, info)
	{}
	bool handle(ARG_VECTOR args) override
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
	bool handle(ARG_VECTOR args) override;
};
