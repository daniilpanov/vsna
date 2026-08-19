cmake_minimum_required(VERSION 3.16)
project(vsna VERSION 1.0.0 LANGUAGES CXX)

# Settings
set(CMAKE_CXX_STANDARD 23)
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
    ${COMMON_DIR}/addr.cpp
    ${COMMON_DIR}/addr.h
    ${COMMON_DIR}/config.cpp 
    ${COMMON_DIR}/config.h
)
set(SERVER_SOURCES
    ${SERVER_DIR}/server.cpp
    ${SERVER_DIR}/server.h
    ${SERVER_DIR}/server_cli.cpp
    ${SERVER_DIR}/server_cli.h
    ${SERVER_DIR}/session.cpp
)
set(CLIENT_SOURCES
    ${CLIENT_DIR}/client.cpp
    ${CLIENT_DIR}/session.cpp
    ${CLIENT_DIR}/cli/client_cli.cpp
    ${CLIENT_DIR}/cli/menu.cpp
    ${CLIENT_DIR}/cli/command_manager.cpp
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
    add_definitions(-DHAS_BOOST -D_WIN32_WINNT=0x0A00)
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
target_include_directories(common PUBLIC ${COMMON_DIR})
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
    ${CLIENT_DIR}/cli 
    ${COMMON_DIR}
)

# Copy config.json
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_SOURCE_DIR}/config.json
            $<TARGET_FILE_DIR:${PROJECT_NAME}>/config.json
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