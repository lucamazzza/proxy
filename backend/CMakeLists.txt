cmake_minimum_required(VERSION 3.16)

project(backend LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Network WebSockets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Network WebSockets)

add_executable(backend
  main.cpp
  backendconfig.h
  backendconfig.cpp
  backendbootstrap.h
  backendbootstrap.cpp
  backendservice.h
  backendservice.cpp
)
target_include_directories(backend PRIVATE
    ${CMAKE_SOURCE_DIR}/appcomm
)

target_link_libraries(backend PRIVATE
    appcomm
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::WebSockets
)

include(GNUInstallDirs)
install(TARGETS backend
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
