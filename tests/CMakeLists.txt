cmake_minimum_required(VERSION 3.21)

project(im_api_tests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 6.8 REQUIRED COMPONENTS Core Network Sql Test)

qt_add_executable(im_api_tests
    main.cpp
    common/ApiClient.cpp
    common/ApiClient.h
    common/TestDatabase.cpp
    common/TestDatabase.h
    common/TestHelpers.cpp
    common/TestHelpers.h
    unit/AuthTest.cpp
    unit/UserTest.cpp
    unit/FriendTest.cpp
    unit/MessageTest.cpp
    integration/FlowTest.cpp
)

target_link_libraries(im_api_tests
    PRIVATE
        Qt6::Core
        Qt6::Network
        Qt6::Sql
        Qt6::Test
)

target_compile_definitions(im_api_tests
    PRIVATE
        TEST_BACKEND_BASE_URL="http://127.0.0.1:8080"
        TEST_DB_CONFIG_PATH="${CMAKE_CURRENT_SOURCE_DIR}/config/test_config.ini"
)

enable_testing()
add_test(NAME im_api_tests COMMAND im_api_tests)
