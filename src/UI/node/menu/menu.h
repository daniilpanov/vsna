#pragma once
#include <algorithm>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#include "helper.h"
#include "node_api.h"

struct CommandInfo
{
	std::string name;
	std::string description;
	std::string usage{ "" };
};

class MenuItem {
  protected:
	NodeApi& _api;

  public:
	const CommandInfo _info;

	virtual ~MenuItem() = default;
	MenuItem(NodeApi& api, const CommandInfo& info) : _api(api), _info(info)
	{}
	virtual bool handle(const std::vector<std::string>&) = 0;
};

class ConnectCommand : public MenuItem {
  public:
	ConnectCommand(NodeApi& api, const CommandInfo& info) : MenuItem(api, info)
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
		_api.connect(parts[0], parts[1]);
		return false;
	};
};

class AddCommand : public MenuItem {
  public:
	AddCommand(NodeApi& api, const CommandInfo& info) : MenuItem(api, info)
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
		_api.addPeer(parts[0], parts[1]);
		std::cout << "[~] Peer added: " << args[0] << std::endl;
		return false;
	};
};

class PeersCommand : public MenuItem {
  public:
	PeersCommand(NodeApi& api, const CommandInfo& info) : MenuItem(api, info)
	{}
	bool handle(const std::vector<std::string>& args) override
	{
		auto known = _api.knownPeers();
		auto connected = _api.connectedPeers();
		if (known.empty())
		{
			std::cout << "[=] No known peers.\n";
			return false;
		}
		std::cout << "[=] Known peers (" << known.size() << "):" << std::endl;
		for (const auto& addr : known)
		{
			bool isConn = std::find(connected.begin(), connected.end(), addr) != connected.end();
			std::cout << "\t" << addr << (isConn ? " [connected]" : "") << std::endl;
		}
		return false;
	};
};

class ConnectAllCommand : public MenuItem {
  public:
	ConnectAllCommand(NodeApi& api, const CommandInfo& info) : MenuItem(api, info)
	{}
	bool handle(const std::vector<std::string>& args) override
	{
		_api.connectToAllKnown();
		return false;
	};
};

class SendCommand : public MenuItem {
  public:
	SendCommand(NodeApi& api, const CommandInfo& info) : MenuItem(api, info)
	{}
	bool handle(const std::vector<std::string>& args) override
	{
		// send <peer addr> <path>  -> to a specific connected peer
		// send <path>              -> to the first connected peer
		std::string peer;
		std::string path;
		if (args.size() == 2)
		{
			peer = args[0];
			path = args[1];
		}
		else if (args.size() == 1)
		{
			path = args[0];
		}
		else
		{
			std::cerr << "[!] Usage: " << _info.usage << std::endl;
			return false;
		}
		_api.sendFile(peer, path);
		return false;
	};
};

class MyPathCommand : public MenuItem {
  public:
	MyPathCommand(NodeApi& api, const CommandInfo& info) : MenuItem(api, info)
	{}
	bool handle(const std::vector<std::string>& args) override
	{
		std::cout << "[=] Node path: " << _api.path() << std::endl;
		return false;
	};
};

class PrintCommand : public MenuItem {
  public:
	PrintCommand(NodeApi& api, const CommandInfo& info) : MenuItem(api, info)
	{}
	bool handle(const std::vector<std::string>& args) override
	{
		std::cout << _api.describe() << std::endl;
		return false;
	};
};

class ExitCommand : public MenuItem {
  public:
	ExitCommand(NodeApi& api, const CommandInfo& info) : MenuItem(api, info)
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
	HelpCommand(NodeApi& api, const CommandInfo& info, CommandManager& manager)
	    : MenuItem(api, info), _manager(manager)
	{}
	bool handle(const std::vector<std::string>& args) override;
};
