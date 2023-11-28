cmake_minimum_required(VERSION 3.10)

project(MizzouDining)

find_package(OpenSSL REQUIRED)

include_directories(${OPENSSL_INCLUDE_DIR})

set(CMAKE_CXX_STANDARD 11)

find_package(LibXml2 REQUIRED)

include_directories(${LIBXML2_INCLUDE_DIR})

add_executable(MizzouDining
    MizzouDining.cpp
    httplib.cc
)

target_link_libraries(MizzouDining ${LIBXML2_LIBRARIES})



