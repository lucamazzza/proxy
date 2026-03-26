cmake_minimum_required(VERSION 3.16)

project(appcommtest LANGUAGES CXX)

enable_testing()

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Test)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Test)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS WebSockets)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(TEST_FILES
    tst_appwritesdk.cpp
    tst_ratelimiter.cpp
    tst_recentmessagecache.cpp
    tst_recoverymanager.cpp
    tst_models.cpp
    tst_errormodel.cpp
    tst_messageprocessor.cpp
    # tst_appcommclient.cpp
)

file(GLOB APPCOMM_SOURCES ../appcomm/*.cpp)
file(GLOB APPCOMM_HEADERS ../appcomm/*.h)

foreach(TEST_SOURCE ${TEST_FILES})
    get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)
    add_executable(${TEST_NAME} 
        ${TEST_SOURCE}
        ${APPCOMM_SOURCES}
    )
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
    target_link_libraries(${TEST_NAME} PRIVATE Qt${QT_VERSION_MAJOR}::Test)
    target_link_libraries(${TEST_NAME} PRIVATE Qt${QT_VERSION_MAJOR}::Network)
    target_link_libraries(${TEST_NAME} PRIVATE Qt${QT_VERSION_MAJOR}::WebSockets)
    target_include_directories(${TEST_NAME} PRIVATE ../appcomm)
endforeach()

