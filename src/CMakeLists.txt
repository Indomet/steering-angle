cmake_minimum_required(VERSION 3.2)
project(helloworld)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_BUILD_TYPE MinSizeRel)

add_executable(${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/helloworld.cpp ${CMAKE_CURRENT_SOURCE_DIR}/helloworld.cpp)

enable_testing()
add_executable(${PROJECT_NAME}-Runner Test-helloworld.cpp ${CMAKE_CURRENT_SOURCE_DIR}/helloworld.cpp)
add_test(NAME ${PROJECT_NAME}-Runner COMMAND ${PROJECT_NAME}-Runner)
set(CMAKE_CXX_FLAGS " ${CMAKE_CXX_FLAGS} --coverage")
