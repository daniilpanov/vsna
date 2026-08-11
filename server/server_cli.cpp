#include "server_cli.h"

void ServerCLI::CLIParse(int argc, char** argv) {
    CLI::App app{ "VSNA Server" };

    std::string ip{ "0.0.0.0" }; // mask all interfaces
    std::string port{ "5555" };
    std::string path{ "/" };
    std::string configFile;
    
    app.add_option("-i,--ip", ip, "IP address of the listener socket");
    app.add_option("-p,--port", port, "Port of the listener socket");
    app.add_option("-d,--dir", path, "Server path to download files or send from");
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
                this->_server.setConfig(Config::loadFromFile(configFile));
            } catch (const std::exception& e) {
                std::cerr << e.what() << std::endl;
                exit(-1);
            }
        } else {
            std::cerr << "[!] File not found: " << configFile << std::endl;
            exit(-1);
        }
    } else {
        this->_server.setConfig(Config(Addr(ip, port), path));
    }
}

void ServerCLI::run(int argc, char** argv) {
    this->CLIParse(argc, argv);
    this->_server.start();
}
