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

if(NOT EMSCRIPTEN)
  find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS WebSockets)
endif()

add_library(appcomm STATIC
  appwritesdk.h
  appwritesdk.cpp
  ratelimiter.h
  ratelimiter.cpp
  recentmessagecache.h
  recentmessagecache.cpp
  recoverymanager.h
  recoverymanager.cpp
  model.h
  errormodel.h
  model.cpp
  errormodel.cpp
  realtime.h
  realtime.cpp
  appcommclient.h
  appcommclient.cpp
  garbagecollector.h
  garbagecollector.cpp
  appcommserver.h
  appcommserver.cpp
  membershipservice.h
  membershipservice.cpp
  messagequeryservice.h
  messagequeryservice.cpp
  state.h
  messageprocessor.h
  messageprocessor.cpp
)

target_link_libraries(appcomm
  PUBLIC
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Network
)

if(EMSCRIPTEN)
  target_compile_definitions(appcomm PUBLIC APPCOMM_NO_WEBSOCKETS)
else()
  target_link_libraries(appcomm PUBLIC Qt${QT_VERSION_MAJOR}::WebSockets)
endif()

target_compile_definitions(appcomm PRIVATE APPCOMM_LIBRARY)
