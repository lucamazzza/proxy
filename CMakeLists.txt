cmake_minimum_required(VERSION 3.16)

project(proxy LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(GNUInstallDirs)
add_subdirectory(appcomm)
add_subdirectory(frontend)
add_subdirectory(backend)
add_subdirectory(test)
