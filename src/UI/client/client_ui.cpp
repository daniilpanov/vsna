#include "client_ui.h"

std::vector<std::string> ClientUI::CLIParse(int argc, char **argv)
{
	CLI::App app{ "VSNA Client" };
	app.allow_extras();

	std::string ip{ "127.0.0.1" }; // localhost
	std::string port{ "5555" };
	std::string path{ "/" };
	std::string configFile;

	app.add_option("-i,--ip", ip, "IP address of the server");
	app.add_option("-p,--port", port, "Port of the server");
	app.add_option("-d,--dir", path, "Client path to download files or send from");
	app.add_option("-c,--config", configFile, "Path to the config file");

	try
	{
		app.parse(argc, argv);
	}
	catch (const CLI::ParseError& e)
	{
		if (dynamic_cast<const CLI::Success*>(&e) != nullptr)
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
				this->_client.setConfig(Config::loadFromFile(configFile));
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
		this->_client.setConfig(Config(Addr(ip, port), path));
	}

	return app.remaining();
}

std::pair<std::string, std::vector<std::string> > ClientUI::parseArgs(const std::string& input)
{
	std::vector<std::string> args = split(input);
	return { args[0], std::vector<std::string>(args.begin() + 1, args.end()) };
}

void ClientUI::repl()
{
	_client.print();

	std::string input;
	while (true)
	{
		std::cout << "> ";
		std::getline(std::cin, input);
		auto [name, cmdArgs] = parseArgs(input);

		if (name.empty())
			continue;

		if (_commandManager.execute(name, cmdArgs))
			break;
	}
}

void ClientUI::run(int argc, char **argv)
{
	auto extras = this->CLIParse(argc, argv);
	_commandManager.initCommands();

	if (!extras.empty())
	{
		std::string name = extras[0];
		std::vector<std::string> cmdArgs(extras.begin() + 1, extras.end());
		_commandManager.execute(name, cmdArgs);
		return;
	}

	repl();
}
