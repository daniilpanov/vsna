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

# Paths
set(PROJECT_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
set(SRC_DIR      ${PROJECT_ROOT}/src)
set(CORE_DIR     ${SRC_DIR}/Core)
set(UI_DIR       ${SRC_DIR}/UI)

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
add_library(utils STATIC ${CORE_DIR}/utils/addr/addr.cpp
                          ${CORE_DIR}/utils/config/config.cpp
                          ${CORE_DIR}/utils/logger/logger.h
                          ${CORE_DIR}/utils/helper/helper.h)
target_include_directories(utils PUBLIC
    ${CORE_DIR}/common/types
    ${CORE_DIR}/common/message
    ${CORE_DIR}/utils
    ${CORE_DIR}/utils/addr
    ${CORE_DIR}/utils/config
    ${CORE_DIR}/utils/logger
    ${CORE_DIR}/utils/helper
)
target_include_directories(utils PUBLIC ${Boost_INCLUDE_DIRS})
target_link_libraries(utils PUBLIC Boost::headers)

# Core node (symmetric peer) + UI
add_library(node STATIC ${CORE_DIR}/node/node.cpp
                        ${CORE_DIR}/node/session.cpp)
target_link_libraries(node PUBLIC utils)
target_include_directories(node PUBLIC
    ${CORE_DIR}/node
    ${CORE_DIR}/common/types)

add_library(ui STATIC ${UI_DIR}/node/node_ui.cpp
                      ${UI_DIR}/node/menu/menu.cpp
                      ${UI_DIR}/node/com_manager/command_manager.cpp)
target_link_libraries(ui PUBLIC node utils CLI11::CLI11)
target_include_directories(ui PUBLIC
    ${UI_DIR}/node
    ${UI_DIR}/node/menu
    ${UI_DIR}/node/com_manager)

add_executable(vsna ${SRC_DIR}/main.cpp)
set_target_properties(vsna PROPERTIES OUTPUT_NAME vsna)
target_link_libraries(vsna PRIVATE ui node utils)
target_include_directories(vsna PRIVATE ${UI_DIR}/node)

# Tests
# GTest is not required to build/run the node. When it is available (native
# package on Termux, or gtest in the vcpkg manifest elsewhere) the unit and
# echo tests are built and registered with CTest.
find_package(GTest QUIET)
if(GTest_FOUND)
    enable_testing()

    set(TEST_DIR ${PROJECT_ROOT}/tests)

    add_executable(vsna_tests
        ${TEST_DIR}/test_helper.cpp
        ${TEST_DIR}/test_addr.cpp
        ${TEST_DIR}/test_config.cpp
        ${TEST_DIR}/test_message.cpp
        ${TEST_DIR}/test_session_routing.cpp)
    target_link_libraries(vsna_tests PRIVATE node utils GTest::gtest_main GTest::gtest)
    target_include_directories(vsna_tests PRIVATE
        ${CORE_DIR}/node
        ${CORE_DIR}/common/types
        ${CORE_DIR}/utils
        ${CORE_DIR}/utils/addr
        ${CORE_DIR}/utils/config
        ${CORE_DIR}/utils/helper)

    add_test(NAME vsna_tests COMMAND vsna_tests)
    set_tests_properties(vsna_tests PROPERTIES TIMEOUT 60)
endif()
