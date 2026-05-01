cmake_minimum_required(VERSION 3.21)

project(im_backend LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 6.8 REQUIRED COMPONENTS Core Network Sql HttpServer WebSockets)

qt_standard_project_setup()

qt_add_executable(im_backend
    main.cpp
    api/HttpServer.cpp
    api/HttpServer.h
    core/Config.cpp
    core/Config.h
    core/Database.cpp
    core/Database.h
    core/JwtHelper.cpp
    core/JwtHelper.h
    models/UserModel.cpp
    models/UserModel.h
    models/MessageModel.cpp
    models/MessageModel.h
    models/FriendshipModel.cpp
    models/FriendshipModel.h
    routers/AuthRouter.cpp
    routers/AuthRouter.h
    routers/MessageRouter.cpp
    routers/MessageRouter.h
    routers/FriendRouter.cpp
    routers/FriendRouter.h
    routers/UserRouter.cpp
    routers/UserRouter.h
    realtime/WebSocketGateway.cpp
    realtime/WebSocketGateway.h
    services/AuthService.cpp
    services/AuthService.h
    services/MessageService.cpp
    services/MessageService.h
    services/FriendService.cpp
    services/FriendService.h
    services/UserService.cpp
    services/UserService.h
)

target_link_libraries(im_backend
    PRIVATE
        Qt6::Core
        Qt6::Network
        Qt6::Sql
        Qt6::HttpServer
        Qt6::WebSockets
)
