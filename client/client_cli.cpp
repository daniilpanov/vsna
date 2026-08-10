#include "client_cli.h"

void ClientCLI::CLIParse(int argc, char** argv) {
    CLI::App app{ "VSNA" };

    std::string ip{ "0.0.0.0" };
    std::string port{ "5555" };
    std::string path{ "/" };
    std::string configFile;
    
    app.add_option("-i,--ip", ip, "IP address of the server");
    app.add_option("-p,--port", port, "Port of the server");
    app.add_option("-d,--dir", path, "Client path to download files or send from");
    app.add_option("-c,--config", configFile, "Path to the config file");

    try {
        app.parse(argc, argv);
    } catch (const CLI::ParseError& e) {
        app.exit(e);
        std::cerr << e.what() << std::endl;
        exit(-1);
    }

    if (!configFile.empty()) {
        if (std::filesystem::exists(configFile)) {
            try {
                this->_config = Config::loadFromFile(configFile);
            } catch (const std::exception& e) {
                std::cerr << e.what() << std::endl;
                exit(-1);
            }
        } else {
            std::cerr << "[!] File not found: " << configFile << std::endl;
            exit(-1);
        }
    } else {
        this->_config = Config(Addr(ip, port), path);
    }
}

void ClientCLI::buildCommands() {
    auto add = [&](auto cmd) {
        _commands[std::string(cmd->getName())] = std::move(cmd);
    };
    add(std::make_unique<HelpCommand>(_client, _commands));
    add(std::make_unique<ExitCommand>(_client, _isExit));
    add(std::make_unique<PrintCommand>(_client));
    add(std::make_unique<MyPathCommand>(_client));
    add(std::make_unique<ConnectCommand>(_client));
    add(std::make_unique<ShowPathCommand>(_client));
    add(std::make_unique<DownloadCommand>(_client));
    add(std::make_unique<SendFilesCommand>(_client));
}

void ClientCLI::run(int argc, char** argv) {
    this->CLIParse(argc, argv);
    _client.setConfig(_config);
    this->buildCommands();
    _client.print();
    
    std::string input;
    while (true) {
        std::cout << "> ";
        std::getline(std::cin, input);
        ARG_VECTOR args = splitArgs(input);

        if (args.empty()) continue;

        auto it = _commands.find(args[0]);
        if (it == _commands.end()) {
            std::cout << "Unknown command: " << args[0] << std::endl;
        } else {
            it->second->handle(ARG_VECTOR(args.begin() + 1, args.end()));
        }
        if (_isExit) break;
    }
}
