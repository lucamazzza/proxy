cmake_minimum_required(VERSION 3.16)

project(frontend VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS
    Quick
    QuickControls2
    Core
    Qml
    Network
    WebSockets
)

qt_policy(SET QTP0001 NEW)

qt_add_executable(appfrontend
    main.cpp
    controller/DemoController.cpp
    controller/DemoController.h
    model/MessageListModel.cpp
    model/MessageListModel.h
)

qt_add_qml_module(appfrontend
    URI frontend
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/LoginChoicePage.qml
        qml/ChatPage.qml
        qml/MessageDelegate.qml
        qml/EmailLoginPage.qml
)

set_target_properties(appfrontend PROPERTIES
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_include_directories(appfrontend PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(appfrontend
    PRIVATE
        Qt6::Quick
        Qt6::QuickControls2
        Qt6::Qml
        appcomm
)

include(GNUInstallDirs)
install(TARGETS appfrontend
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)