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
set(COMMON_DIR   ${PROJECT_ROOT}/common)
set(SERVER_DIR   ${PROJECT_ROOT}/server)
set(CLIENT_DIR   ${PROJECT_ROOT}/client)

# Sources
# TOOO : Can be replaced with file(GLOB_RECURSE)
set(COMMON_SOURCES
    ${COMMON_DIR}/addr/addr.cpp
    ${COMMON_DIR}/config/config.cpp 
    ${COMMON_DIR}/logger/logger.cpp
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

# Core
file(GLOB_RECURSE CORE_SRCS "${PROJECT_ROOT}/core/*.cpp")
file(GLOB_RECURSE CORE_HDRS "${PROJECT_ROOT}/core/*.h")
add_executable(${PROJECT_NAME} ${CORE_SRCS} ${CORE_HDRS})
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

# Common
add_library(common STATIC ${COMMON_SOURCES})
target_include_directories(common PUBLIC
    ${COMMON_DIR}
    ${COMMON_DIR}/addr
    ${COMMON_DIR}/config
    ${COMMON_DIR}/logger
)
target_link_libraries(common PUBLIC Boost::headers ${Boost_LIBRARIES})
target_link_libraries(${PROJECT_NAME} PRIVATE common)

# Server
add_library(server STATIC ${SERVER_SOURCES})
target_link_libraries(server PRIVATE common)
target_include_directories(server PUBLIC ${SERVER_DIR} ${COMMON_DIR})

# Client
add_library(client STATIC ${CLIENT_SOURCES})
target_link_libraries(client PRIVATE common)
target_include_directories(client PUBLIC
    ${CLIENT_DIR} 
    ${CLIENT_DIR}/ui
    ${CLIENT_DIR}/menu
    ${CLIENT_DIR}/com_manager
    ${CLIENT_DIR}/session
    ${COMMON_DIR}
)

# Compile definitions
if (SERVER)
    target_compile_definitions(${PROJECT_NAME} PRIVATE BUILD_SERVER=1)
    target_link_libraries(${PROJECT_NAME} PRIVATE server)
    target_include_directories(${PROJECT_NAME} PRIVATE ${SERVER_DIR})
elseif (CLIENT)
    target_compile_definitions(${PROJECT_NAME} PRIVATE BUILD_CLIENT=1)
    target_link_libraries(${PROJECT_NAME} PRIVATE client)
    target_include_directories(${PROJECT_NAME} PRIVATE ${CLIENT_DIR})
else()
    message(FATAL_ERROR "Either SERVER or CLIENT must be ON")
endif()
