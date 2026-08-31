# VSNA
**Virtual Storage and Network Access** is open-source CLI project, implemented on C++, to exchange data between devices on a _VLAN_.

# Dependencies
All dependencies are managed through `vcpkg`:
- `boost` - asio + beast (_websocket_);
- `CLI11` - command line interface parser;
- `nlohmann/json` - JSON parsing library.

# Build
Initialize the `vcpkg` submodule (first time only), then configure and build.
Make automatically configure the project and init submodule `vcpkg`.
CMake auto-bootstraps `vcpkg` and installs all dependencies (declared in `vcpkg.json`).
See the commands:

```bash
make configure     # vcpkg bootstrap + dependency install + configure
make build         # build the targets
```

`make configure` calls `cmake --preset default` that builds both `vsna_server` and `vsna_client` into `out/`.
To build only the server or client, pass `-DBUILD_SERVER_EXE=OFF` / `-DBUILD_CLIENT_EXE=OFF`
to the `cmake --preset default` after `make configure`

# To run
Binaries land in `out/` (on Windows multi-config builds add `Debug\ ` subfolder).

**Client**

- With CLI flags:

```bash
./out/vsna_client -i 127.0.0.1 -p 5555 -d
```

- With config file:

```bash
./out/vsna_client -c ./config/config.example.json
```

**Server**

- With CLI flags:

```bash
./out/vsna_server -i 0.0.0.0 -p 5555 -d
```

- With config file:

```bash
./out/vsna_server -c ./config/config.example.json
```

**CLI Scheme**

|Short, Long name|Description|Default value|
|---|---|---|
| `-h`, `--help` | show help message |-|
| `-p`, `--port <port>` | set port | 5555 |
| `-i`, `--ip <ip>` | set client/server address | 0.0.0.0 |
| `-d`, `--dir <path>` | set client/server path | <current directory> |
| `-c`, `--config <path>` | set config file path | none |

**Project Tree**
```
vsna/
├── .clang-format              # правила форматирования кода
├── Makefile                   # хелпер форматирования и сборки (format/configure/build)
├── .gitignore
├── vcpkg.json                  # манифест зависимостей vcpkg (Boost, CLI11, nlohmann-json)
├── CMakeLists.txt             # корневой сценарий сборки (цели: vsna_server + vsna_client + utils/client/server libs)
├── CMakePresets.json          # пресеты сборки (default = обе цели через vcpkg)
├── README.md
│
├── config/                    # конфиги приложения
│   └── config.example.json    # шаблон для новых развёртываний
│
└── src/                       # весь исходный код
    ├── main.cpp               # точка входа; BUILD_SERVER/BUILD_CLIENT выбирают роль
    │
    ├── client/                # КЛИЕНТСКАЯ ЧАСТЬ
    │   ├── client.{h,cpp}     # Client: io_context, connect/sendFiles/download (stub'ы)
    │   ├── session/           # исходящий WebSocket-сеанс (ClientSession, пока one-shot)
    │   ├── ui/
    │   │   └── client_ui.*    # ClientUI: CLI11-парсинг, REPL-цикл, владеет CommandManager
    │   ├── menu/              # MenuItem-иерархия: классы-команды (connect, help, exit...)
    │   └── com_manager/       # CommandManager: реестр и вызов команд
    │
    ├── server/                # СЕРВЕРНАЯ ЧАСТЬ (цель server.lib)
    │   ├── server_cli.*       # ServerCLI: CLI11-парсинг, владеет Server
    │   ├── server.*           # Server: acceptor + пул потоков, цикл приёма соединений
    │   └── session.*          # ServerSession: WS-сессия клиента (read => echo => read)
    │
    ├── common/types/          # общие типы
    │   ├── types.h            # STRING_ARG, ARG_VECTOR и др. алиасы
    │   └── pch.h              # precompiled header: boost/beast алиасы, fail()
    │
    └── utils/                 # утилиты общего назначения (цель utils.lib)
        ├── addr/              # Addr: ip:port, валидация, toString
        ├── config/            # Config: загрузка из json, getAddr/getPath
        ├── helper/helper.h    # inline-утилиты: trim, splitArgs, isValidIPv4
        └── logger/            # Logger: файловый лог с уровнями
```
