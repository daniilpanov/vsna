# VSNA
**Virtual Storage and Network Access** is open-source CLI project, implemented on C++, to exchange data between devices on a _VLAN_.

# Dependencies
- `boost` - asio + beast (_websocket_);
- `CLI11` - command line interface parser;
- `nlohmann/json` - JSON parsing library.

# Build
First you need to initialize boost via `vcpkg`.

If you don't have it, you can install it by running:

```bash
.\init_modules.bat --init # See more flags with --help
```

**Default build**

Then you can build the project (for client or server):

```bash
.\build.bat [--client|--server]
```

Otherwise, if you do not specify the flag, both configurations will be built.

If you use Unix system, you can do the same actions via Shell scripts:

```bash
./init_modules.sh
./build.sh # See more flags with --help
```

# To run
**Client**

- With CLI flags:

```bash
.\out\client\Debug\vsna_client.exe -i 127.0.0.1 -p 5555 -d \
```

- With config file:

```bash
.\out\client\Debug\vsna_client.exe -c config.example.json
```

**Server**

- With CLI flags:

```bash
.\out\server\Debug\vsna_server.exe -i 0.0.0.0 -p 5555 -d \
```

- With config file:

```bash
.\out\server\Debug\vsna_server.exe -c config.example.json
```

**CLI Scheme**

|Short, Long name|Description|Default value|
|---|---|---|
| `-h`, `--help` | show help message |-|
| `-p`, `--port <port>` | set port | 8080 |
| `-i`, `--ip <ip>` | set client/server address | 0.0.0.0 |
| `-d`, `--dir <path>` | set client/server path | <current directory> |
| `-c`, `--config <path>` | set config file path | none |