cmake_minimum_required(VERSION 3.10)
project(WinTex)

set(CMAKE_CXX_STANDARD 17)

if(WIN32)
    file(GLOB SOURCES "*.cpp")
    add_definitions(-DPLATFORM_WINDOWS)
else()
    file(GLOB SOURCES "*.cpp")
    list(REMOVE_ITEM SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/WinTex.cpp")
    add_definitions(-DPLATFORM_LINUX)
endif()

add_executable(WinTex ${SOURCES})

if(NOT WIN32)
    target_link_libraries(WinTex m)
endif()
