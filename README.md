# VSNA
**Virtual Storage and Network Access** is open-source CLI project, implemented on C++, to exchange data between devices on a _VLAN_.

# Dependencies
- `boost` - asio + beast (_websocket_);
- `CLI11` - command line interface parser;
- `nlohmann/json` - JSON parsing library;
- `cpptui` - text based user interface library.

# Build
First initialize the `vcpkg` submodule and install Boost:

```bash
git submodule update --init vcpkg
./init_modules.sh          # Unix
.\init_modules.bat --init  # Windows
```

The header-only libraries (CLI11, nlohmann/json, cpptui) are already committed in `libs/`.

Then configure and build both `vsna_server` and `vsna_client` with CMake:

```bash
cmake --preset default     # or: cmake -B out
cmake --build --preset default
```

Binaries are written to `out/`. To build only the server or client, pass
`-DBUILD_SERVER_EXE=OFF` / `-DBUILD_CLIENT_EXE=OFF` to the configure step.

# To run
**Client**

- With CLI flags:

```bash
.\out\client\Debug\vsna_client.exe -i 127.0.0.1 -p 5555 -d \
```

- With config file:

```bash
.\out\client\Debug\vsna_client.exe -c .\config\config.example.json
```

**Server**

- With CLI flags:

```bash
.\out\server\Debug\vsna_server.exe -i 0.0.0.0 -p 5555 -d \
```

- With config file:

```bash
.\out\server\Debug\vsna_server.exe -c .\config\config.example.json
```

**CLI Scheme**

|Short, Long name|Description|Default value|
|---|---|---|
| `-h`, `--help` | show help message |-|
| `-p`, `--port <port>` | set port | 8080 |
| `-i`, `--ip <ip>` | set client/server address | 0.0.0.0 |
| `-d`, `--dir <path>` | set client/server path | <current directory> |
| `-c`, `--config <path>` | set config file path | none |

**Project Tree**
```
vsna/
├── .clang-format              # правила форматирования кода
├── Makefile                   # хелпер форматирования кода
├── .gitignore
├── init_modules.bat / .sh     # инициализация зависимостей (vcpkg + Boost)
├── CMakeLists.txt             # корневой сценарий сборки (цели: vsna_server + vsna_client + utils/client/server libs)
├── CMakePresets.json          # пресеты сборки (default = обе цели через vcpkg)
├── README.md
│
├── config/                    # конфиги приложения
│   └── config.example.json    # шаблон для новых развёртываний
│
├── libs/                      # header-only сторонние библиотеки
│   ├── CLI11.hpp              # парсер аргументов командной строки
│   ├── cpptui.hpp             # TUI-фреймворк
│   └── json.hpp               # парсинг config.json
│
└── src/                       # весь исходный код
    ├── main.cpp               # точка входа; BUILD_SERVER/BUILD_CLIENT выбирают роль
    │
    ├── client/                # КЛИЕНТСКАЯ ЧАСТЬ
    │   ├── client.{h,cpp}     # Client: io_context, connect/sendFiles/download (stub'ы)
    │   ├── session/           # исходящий WebSocket-сеанс (ClientSession, пока one-shot)
    │   ├── ui/
    │   │   ├── client_ui.*    # ClientUI: CLI11-парсинг, REPL-цикл, владеет CommandManager
    │   │   └── tui.*          # демо-TUI на cpptui (в сборку не входит, ждёт адаптации)
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
        ├── helpers/helper.h   # inline-утилиты: trim, splitArgs, isValidIPv4
        └── logger/            # Logger: файловый лог с уровнями
```
