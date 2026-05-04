cmake_minimum_required(VERSION 3.21)

project(QQDesktopFrontend VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 6.8.3 REQUIRED COMPONENTS Widgets Network WebSockets)

qt_standard_project_setup()

qt_add_executable(qq_desktop_frontend
  src/main.cpp
  src/ApiClient.cpp
  src/ApiClient.h
  src/AuthClient.cpp
  src/AuthClient.h
  src/ChatListWidget.cpp
  src/ChatListWidget.h
  src/ChatWindow.cpp
  src/ChatWindow.h
  src/MainWindow.cpp
  src/MainWindow.h
  src/MessageBubble.cpp
  src/MessageBubble.h
  src/NavBar.cpp
  src/NavBar.h
  src/TitleBar.cpp
  src/TitleBar.h
  src/LoginWindow.cpp
  src/LoginWindow.h
  widgets/AddFriendDialog.cpp
  widgets/AddFriendDialog.h
  widgets/EditProfileDialog.cpp
  widgets/EditProfileDialog.h
  widgets/FriendListWidget.cpp
  widgets/FriendListWidget.h
  widgets/ImageViewerDialog.cpp
  widgets/ImageViewerDialog.h
  widgets/ProfileDialog.cpp
  widgets/ProfileDialog.h
  widgets/UploadProgressWidget.cpp
  widgets/UploadProgressWidget.h
  resources/MainWindow.qrc
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
