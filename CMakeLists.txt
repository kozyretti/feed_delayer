cmake_minimum_required(VERSION 3.2)
project(ticks_app)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
#set (CMAKE_CXX_STANDARD 17)

set(SOURCE_FILES main.cpp websocket_client.cpp websocket_client.h)

set(Boost_USE_STATIC_LIBS ON)
find_package(Boost REQUIRED COMPONENTS regex)

set(OPENSSL_USE_STATIC_LIBS ON)
find_package(OpenSSL REQUIRED)

include_directories(${BOOST_INCLUDE_DIRS})

add_executable(${PROJECT_NAME} ${SOURCE_FILES})
#target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES} OpenSSL::SSL OpenSSL::Crypto)
target_link_libraries(${PROJECT_NAME} Boost::regex OpenSSL::SSL OpenSSL::Crypto)
