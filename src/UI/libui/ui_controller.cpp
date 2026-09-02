#include "ui_controller.h"

#include <boost/asio.hpp>
#include <CLI/CLI.hpp>
#include <filesystem>
#include <iostream>

#include "helper.h"

std::vector<std::string> UiController::parse(int argc, char **argv)
{
	CLI::App app{ "VSNA Node" };
	app.allow_extras();

	std::string ip{ "0.0.0.0" };
	std::string port{ "5555" };
	std::string path{ "/" };
	std::string configFile;
	std::vector<std::string> connects;

	app.add_option("-i,--ip", ip, "IP address of the listener socket");
	app.add_option("-p,--port", port, "Port of the listener socket");
	app.add_option("-d,--dir", path, "Node path to store and serve files");
	app.add_option("-c,--config", configFile, "Path to the config file");
	app.add_option("--connect", connects, "Connect to a peer at startup (repeatable)");

	try
	{
		app.parse(argc, argv);
	}
	catch (const CLI::ParseError& e)
	{
		if (dynamic_cast<const CLI::Success *>(&e) != nullptr)
		{
			app.exit(e);
			exit(EXIT_SUCCESS);
		}
		app.exit(e);
		std::cerr << e.what() << std::endl;
		exit(-1);
	}

	if (!configFile.empty())
	{
		if (std::filesystem::exists(configFile))
		{
			try
			{
				_api.configureFromFile(configFile);
			}
			catch (const std::exception& e)
			{
				std::cerr << e.what() << std::endl;
				exit(-1);
			}
		}
		else
		{
			std::cerr << "[!] File not found: " << configFile << std::endl;
			exit(-1);
		}
	}
	else
	{
		_api.configure(ip, port, path);
	}

	_startConnects = connects;

	return app.remaining();
}

std::pair<std::string, std::vector<std::string>> UiController::splitLine(const std::string& input)
{
	std::vector<std::string> args = split(input);
	if (args.empty())
		return {};
	return { args[0], std::vector<std::string>(args.begin() + 1, args.end()) };
}

void UiController::start()
{
	_commandManager.initCommands();

	// Notify (and offer to connect to) every newly discovered peer.
	_api.setOnPeerDiscovered([](const std::string& addr) {
		std::cout << "[+] New peer discovered: " << addr << " — type 'connect " << addr
		          << "' to connect\n";
	});

	// Start listening in the background.
	_api.start();

	// Optionally connect to peers at startup (multi-instance orchestration).
	for (const auto& addr : _startConnects)
	{
		auto parts = split(addr, ":");
		if (parts.size() == 2)
			_api.connect(parts[0], parts[1]);
		else
			std::cerr << "[!] Ignoring bad connect target: " << addr << std::endl;
	}
}

void UiController::stop()
{
	_api.stop();
}