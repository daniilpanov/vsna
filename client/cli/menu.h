#pragma once
#include <string>
#include <string_view>
#include <vector>

#include "client.h"
#include "config.h"
#include "types.h"

class MenuItem {
  protected:
	Client& _client;

  public:
	const std::string name;
	const std::string desc;
	const std::string usage{ "" };

	virtual ~MenuItem() = default;
	MenuItem(Client& client, std::string_view name, std::string_view desc, std::string_view usage) 
	    : _client(client), name(name), desc(desc), usage(usage)
	{}
	virtual bool handle(CONST_ARG_VECTOR) = false;
};

class ConnectCommand : public MenuItem {
  public:
	ConnectCommand(Client& client, std::string_view name, std::string_view desc, std::string_view usage) 
	    : MenuItem(client, name, desc, usage)
	{}
	bool handle(CONST_ARG_VECTOR args) override
	{
		_client.connect(args);
		return false;
	};
};

class ShowPathCommand : public MenuItem {
  public:
	ShowPathCommand(Client& client, std::string_view name, std::string_view desc, std::string_view usage)
	    : MenuItem(client, name, desc, usage)
	{}
	bool handle(CONST_ARG_VECTOR args) override
	{
		_client.showPath(args);
		return false;
	};
};

class MyPathCommand : public MenuItem {
  public:
	MyPathCommand(Client& client, std::string_view name, std::string_view desc, std::string_view usage)
	    : MenuItem(client, name, desc, usage)
	{}
	bool handle(CONST_ARG_VECTOR args) override
	{
		_client.myPath(args);
		return false;
	};
};

class SendFilesCommand : public MenuItem {
  public:
	SendFilesCommand(Client& client, std::string_view name, std::string_view desc, std::string_view usage)
	    : MenuItem(client, name, desc, usage)
	{}
	bool handle(CONST_ARG_VECTOR args) override
	{
		_client.sendFiles(args);
		return false;
	};
};

class DownloadCommand : public MenuItem {
  public:
	DownloadCommand(Client& client, std::string_view name, std::string_view desc, std::string_view usage)
	    : MenuItem(client, name, desc, usage)
	{}
	bool handle(CONST_ARG_VECTOR args) override
	{
		_client.download(args);
		return false;
	};
};

class PrintCommand : public MenuItem {
  public:
	PrintCommand(Client& client, std::string_view name, std::string_view desc, std::string_view usage)
	    : MenuItem(client, name, desc, usage)
	{}
	bool handle(CONST_ARG_VECTOR args) override
	{
		_client.print();
		return false;
	};
};

class ExitCommand : public MenuItem {
  public:
	ExitCommand(Client& client, std::string_view name, std::string_view desc, std::string_view usage)
	    : MenuItem(client, name, desc, usage)
	{}
	bool handle(CONST_ARG_VECTOR args) override
	{
		std::cout << "[~] Program was exit." << std::endl;
		return true;
	}
};

class CommandManager;

class HelpCommand : public MenuItem {
	CommandManager& _manager;

  public:
	HelpCommand(Client& client, CommandManager& manager,
	            std::string_view name, std::string_view desc, std::string_view usage)
	    : MenuItem(client, name, desc, usage), _manager(manager)
	{}
	bool handle(CONST_ARG_VECTOR args) override;
};
