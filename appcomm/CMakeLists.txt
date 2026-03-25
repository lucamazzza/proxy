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
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS WebSockets)

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

)

target_link_libraries(appcomm PRIVATE Qt${QT_VERSION_MAJOR}::Core)
target_link_libraries(appcomm PRIVATE Qt${QT_VERSION_MAJOR}::Network)
target_link_libraries(appcomm PRIVATE Qt${QT_VERSION_MAJOR}::WebSockets)
target_compile_definitions(appcomm PRIVATE APPCOMM_LIBRARY)
