cmake_minimum_required(VERSION 3.16)

project(proxy LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(GNUInstallDirs)

option(BUILD_FRONTEND "Build frontend application" ON)
option(BUILD_BACKEND "Build backend application" ON)
option(BUILD_TESTING "Build test executables" ON)

if(BUILD_TESTING)
    enable_testing()
endif()

add_subdirectory(appcomm)
if(BUILD_FRONTEND)
    add_subdirectory(frontend)
endif()
if(BUILD_BACKEND)
    add_subdirectory(backend)
endif()
if(BUILD_TESTING)
    add_subdirectory(test)
endif()
