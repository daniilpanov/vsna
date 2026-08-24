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
include(FetchContent)

# Definitions for compilation
option(SERVER "Build server binary" OFF)
option(CLIENT "Build client binary" OFF)

# Paths
set(PROJECT_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
set(SRC_DIR      ${PROJECT_ROOT}/src)
set(UTILS_DIR    ${SRC_DIR}/utils)
set(SERVER_DIR   ${SRC_DIR}/server)
set(CLIENT_DIR   ${SRC_DIR}/client)

# Sources
set(UTILS_SOURCES
    ${UTILS_DIR}/addr/addr.cpp
    ${UTILS_DIR}/config/config.cpp
    ${UTILS_DIR}/logger/logger.cpp
)
set(SERVER_SOURCES
    ${SERVER_DIR}/server.cpp
    ${SERVER_DIR}/server_cli.cpp
    ${SERVER_DIR}/session.cpp
)
set(CLIENT_SOURCES
    ${CLIENT_DIR}/client.cpp
    ${CLIENT_DIR}/session/session.cpp
    ${CLIENT_DIR}/ui/client_ui.cpp
    ${CLIENT_DIR}/menu/menu.cpp
    ${CLIENT_DIR}/com_manager/command_manager.cpp
)

# Main executable : src/main.cpp picks server/client via BUILD_* defines
add_executable(${PROJECT_NAME} ${SRC_DIR}/main.cpp)
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "${APP_NAME}")

# Boost
set(BOOST_REQUIRED_COMPONENTS
    filesystem
    system
    exception
)
find_package(Boost REQUIRED COMPONENTS ${BOOST_REQUIRED_COMPONENTS})
find_package(Threads REQUIRED)

if (Boost_FOUND)
    if (WIN32)
        add_definitions(-DHAS_BOOST -D_WIN32_WINNT=0x0A00 -DWIN32_LEAN_AND_MEAN)
    endif()
    target_include_directories(${PROJECT_NAME} PRIVATE ${Boost_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} PRIVATE
        Boost::headers
        ${Boost_LIBRARIES}
        Threads::Threads
    )
    message("-- Boost status: Found")
else()
    message(FATAL_ERROR "-- Boost status: Not found")
endif()

# Utils : addr/config/logger sources, plus public headers
# (${PROJECT_ROOT} makes <libs/*.hpp> visible, common/types has types.h & pch.h)
add_library(utils STATIC ${UTILS_SOURCES})
target_include_directories(utils PUBLIC
    ${PROJECT_ROOT}
    ${SRC_DIR}/common/types
    ${UTILS_DIR}
    ${UTILS_DIR}/addr
    ${UTILS_DIR}/config
    ${UTILS_DIR}/logger
    ${UTILS_DIR}/helpers
)
target_link_libraries(utils PUBLIC Boost::headers ${Boost_LIBRARIES})
target_link_libraries(${PROJECT_NAME} PRIVATE utils)

# Compile definitions
if (SERVER)
    add_library(server STATIC ${SERVER_SOURCES})
    target_link_libraries(server PRIVATE utils)
    target_include_directories(server PUBLIC ${SERVER_DIR})

    target_compile_definitions(${PROJECT_NAME} PRIVATE BUILD_SERVER=1)
    target_link_libraries(${PROJECT_NAME} PRIVATE server)
    target_include_directories(${PROJECT_NAME} PRIVATE ${SERVER_DIR})
elseif (CLIENT)
    add_library(client STATIC ${CLIENT_SOURCES})
    target_link_libraries(client PRIVATE utils)
    target_include_directories(client PUBLIC
        ${CLIENT_DIR}
        ${CLIENT_DIR}/ui
        ${CLIENT_DIR}/menu
        ${CLIENT_DIR}/com_manager
        ${CLIENT_DIR}/session)

    target_compile_definitions(${PROJECT_NAME} PRIVATE BUILD_CLIENT=1)
    target_link_libraries(${PROJECT_NAME} PRIVATE client)
    target_include_directories(${PROJECT_NAME} PRIVATE ${CLIENT_DIR})
else()
    message(FATAL_ERROR "Either SERVER or CLIENT must be ON")
endif()
