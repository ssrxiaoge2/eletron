cmake_minimum_required(VERSION 3.21)

project(QQDesktopFrontend VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 6.8.3 REQUIRED COMPONENTS Widgets Network WebSockets)

qt_standard_project_setup()

qt_add_executable(qq_desktop_frontend
  src/main.cpp
  src/AuthClient.cpp
  src/AuthClient.h
  src/MainWindow.cpp
  src/MainWindow.h
  src/LoginWindow.cpp
  src/LoginWindow.h
)

target_link_libraries(qq_desktop_frontend
  PRIVATE
    Qt6::Widgets
    Qt6::Network
    Qt6::WebSockets
)

set_target_properties(qq_desktop_frontend PROPERTIES
  WIN32_EXECUTABLE TRUE
)
