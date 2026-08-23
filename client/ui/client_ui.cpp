#include "client_ui.h"

void ClientUI::CLIParse(int argc, char **argv)
{
	CLI::App app{ "VSNA Client" };

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
}

void ClientUI::run(int argc, char **argv)
{
	this->CLIParse(argc, argv);
	_commandManager.initCommands();

	// TUI call
	
	_client.print();

	std::string input;
	while (true)
	{
		std::cout << "> ";
		std::getline(std::cin, input);
		ARG_VECTOR args = splitArgs(input);

		if (args.empty())
			continue;

		std::string name = args[0];
		ARG_VECTOR cmdArgs(args.begin() + 1, args.end());

		if (_commandManager.execute(name, cmdArgs))
			break;
	}
}
