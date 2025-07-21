cmake_minimum_required(VERSION 3.22)

project("QWin11PhantomStyle"
    LANGUAGES CXX
    DESCRIPTION "Qt style, Windows 11 like, built on QPhantomStyle."
)

option(BUILD_PLUGIN "Build the QWin11PhantomStyle plugin." OFF)
option(BUILD_LIBRARY "Build as an embedable library." ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#find_package(Qt6 QUIET COMPONENTS Core Gui Widgets)
#if(NOT Qt6_FOUND)
    find_package(Qt5 5.15 REQUIRED COMPONENTS Core Widgets LinguistTools)
    if(Qt5_FOUND)
        message(STATUS "Found Qt " ${Qt5_VERSION})
    else()
        message(FATAL_ERROR "Could not find Qt (need version >= 5.15)!")
    endif()
#else()
#    message(STATUS "Found Qt " ${Qt6_VERSION})
#    qt_standard_project_setup(REQUIRES 6.4)
#endif()

if(BUILD_PLUGIN)
    #add_subdirectory(styleplugin)
    message(FATAL_ERROR "Not supported: building as a plugin.")
endif()
if(BUILD_LIBRARY)
    add_subdirectory(style)
endif()