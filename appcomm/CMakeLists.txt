cmake_minimum_required(VERSION 3.16)

project(appcomm LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Network)

add_library(appcomm STATIC
  appcomm.cpp
  appcomm.h
  appwritesdk.h
  appwritesdk.cpp
  ratelimiter.h ratelimiter.cpp
  model.h
  errormodel.h
  model.cpp

)

target_link_libraries(appcomm PRIVATE Qt${QT_VERSION_MAJOR}::Core)
target_link_libraries(appcomm PRIVATE Qt${QT_VERSION_MAJOR}::Network)
target_compile_definitions(appcomm PRIVATE APPCOMM_LIBRARY)
