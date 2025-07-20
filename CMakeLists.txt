cmake_minimum_required(VERSION 3.22)

project("QWin11PhantomStyle"
    LANGUAGES CXX
    DESCRIPTION "Qt style, Windows 11 like, built on QPhantomStyle."
)

option(BUILD_PLUGIN "Build the QWin11PhantomStyle plugin." OFF)
option(BUILD_LIBRARY "Build as an embedable library." ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)
qt_standard_project_setup()

if(BUILD_PLUGIN)
    #add_subdirectory(styleplugin)
    message(FATAL_ERROR "Not supported: building as a plugin.")
endif()
if(BUILD_LIBRARY)
    add_subdirectory(style)
endif()