cmake_minimum_required(VERSION 3.16)
project(vsna VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(POLICY CMP0167)
    cmake_policy(SET CMP0167 OLD)
endif()
include(FetchContent)

# set(PROJECT_DEFINITIONS)
option(SERVER "Build server binary" OFF)
option(CLIENT "Build client binary" OFF)

set(PROJECT_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
set(COMMON_DIR   ${PROJECT_ROOT}/common)

set(COMMON_SOURCES
    ${COMMON_DIR}/addr.cpp
    ${COMMON_DIR}/addr.h
    ${COMMON_DIR}/config.cpp 
    ${COMMON_DIR}/config.h 
)

# Common
add_library(common STATIC ${COMMON_SOURCES})
target_include_directories(common PUBLIC ${COMMON_DIR})

# Core
file(GLOB_RECURSE CORE_SRCS "${PROJECT_ROOT}/core/*.cpp")
file(GLOB_RECURSE CORE_HDRS "${PROJECT_ROOT}/core/*.h")

add_executable(${PROJECT_NAME} ${CORE_SRCS} ${CORE_HDRS})
target_link_libraries(${PROJECT_NAME} PRIVATE common)
target_include_directories(${PROJECT_NAME} PRIVATE ${COMMON_DIR})

# Boost
set(BOOST_REQUIRED_COMPONENTS
    filesystem
    system
)
find_package(Boost REQUIRED COMPONENTS ${BOOST_REQUIRED_COMPONENTS})

if (Boost_FOUND)
    add_definitions(-DHAS_BOOST)
    target_include_directories(${PROJECT_NAME} PRIVATE ${Boost_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} PRIVATE 
        Boost::headers
        ${Boost_LIBRARIES}
    )

    message("-- Boost status: Found")
else()
    message(FATAL_ERROR "-- Boost status: Not found")
endif()

# CLI
FetchContent_Declare(
  cli11
  GIT_REPOSITORY https://github.com/CLIUtils/CLI11
  GIT_TAG v2.6.2
)
FetchContent_MakeAvailable(cli11)

set(INCLUDE_DIRS
    common/addr/
    common/config/
)

set(LIB_SOURCES
    common/addr/addr.cpp
    common/config/config.cpp
)

set(LINK_LIBRARIES
    ${Boost_LIBRARIES}
    nlohmann_json::nlohmann_json
    CLI11::CLI11
)

if (SERVER)
    target_compile_definitions(${PROJECT_NAME} PRIVATE BUILD_SERVER=1)
elseif (CLIENT)
    target_compile_definitions(${PROJECT_NAME} PRIVATE BUILD_CLIENT=1)
else()
    message(FATAL_ERROR "Either SERVER or CLIENT must be ON")
endif()