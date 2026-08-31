cmake_minimum_required(VERSION 3.16)
project(vsna VERSION 1.0.0 LANGUAGES CXX)

# Settings
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(POLICY CMP0167)
    cmake_policy(SET CMP0167 OLD)
endif()

# Options (each defaults to ON; both can be built in one configure)
option(BUILD_SERVER_EXE "Build server binary" ON)
option(BUILD_CLIENT_EXE "Build client binary" ON)

# Paths
set(PROJECT_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
set(SRC_DIR      ${PROJECT_ROOT}/src)
set(UTILS_DIR    ${SRC_DIR}/utils)
set(SERVER_DIR   ${SRC_DIR}/server)
set(CLIENT_DIR   ${SRC_DIR}/client)

# Boost
# VSNA uses Boost header-only libraries (Asio, Beast, system headers),
# so only the headers target is required. This works both with vcpkg (PC)
# and with system-packaged Boost (e.g. Termux), where the component .so
# libs (libboost_system, libboost_exception) may not exist.
find_package(Boost REQUIRED)
find_package(Threads REQUIRED)
find_package(CLI11 CONFIG REQUIRED)

if (Boost_FOUND)
    if (WIN32)
        add_definitions(-DHAS_BOOST -D_WIN32_WINNT=0x0A00 -DWIN32_LEAN_AND_MEAN)
    endif()
    message("-- Boost status: Found")
else()
    message(FATAL_ERROR "-- Boost status: Not found")
endif()

# Utils (always)
add_library(utils STATIC ${SRC_DIR}/utils/addr/addr.cpp
                          ${SRC_DIR}/utils/config/config.cpp
                          ${SRC_DIR}/utils/logger/logger.h
                          ${SRC_DIR}/utils/helper/helper.h)
target_include_directories(utils PUBLIC
    ${SRC_DIR}/common/types
    ${UTILS_DIR}
    ${UTILS_DIR}/addr
    ${UTILS_DIR}/config
    ${UTILS_DIR}/logger
    ${UTILS_DIR}/helper
)
target_include_directories(utils PUBLIC ${Boost_INCLUDE_DIRS})
target_link_libraries(utils PUBLIC Boost::headers)

# Helper to define a binary (server or client)
function(add_vsna_binary kind)
    if(kind STREQUAL "server")
        add_library(server STATIC ${SERVER_DIR}/server.cpp
                                  ${SERVER_DIR}/server_cli.cpp
                                  ${SERVER_DIR}/session.cpp)
        target_link_libraries(server PUBLIC utils CLI11::CLI11)
        target_include_directories(server PUBLIC ${SERVER_DIR})

        add_executable(vsna_server ${SRC_DIR}/main.cpp)
        set_target_properties(vsna_server PROPERTIES OUTPUT_NAME vsna_server)
        target_compile_definitions(vsna_server PRIVATE BUILD_SERVER=1)
        target_link_libraries(vsna_server PRIVATE server utils)
        target_include_directories(vsna_server PRIVATE ${SERVER_DIR})
    else()
        add_library(client STATIC ${CLIENT_DIR}/client.cpp
                                  ${CLIENT_DIR}/session/session.cpp
                                  ${CLIENT_DIR}/ui/client_ui.cpp
                                  ${CLIENT_DIR}/menu/menu.cpp
                                  ${CLIENT_DIR}/com_manager/command_manager.cpp)
        target_link_libraries(client PUBLIC utils CLI11::CLI11)
        target_include_directories(client PUBLIC
            ${CLIENT_DIR}
            ${CLIENT_DIR}/ui
            ${CLIENT_DIR}/menu
            ${CLIENT_DIR}/com_manager
            ${CLIENT_DIR}/session)

        add_executable(vsna_client ${SRC_DIR}/main.cpp)
        set_target_properties(vsna_client PROPERTIES OUTPUT_NAME vsna_client)
        target_compile_definitions(vsna_client PRIVATE BUILD_CLIENT=1)
        target_link_libraries(vsna_client PRIVATE client utils)
        target_include_directories(vsna_client PRIVATE ${CLIENT_DIR})
    endif()
endfunction()

if(BUILD_SERVER_EXE)
    add_vsna_binary(server)
endif()
if(BUILD_CLIENT_EXE)
    add_vsna_binary(client)
endif()
