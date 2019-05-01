cmake_minimum_required(VERSION 2.8)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "-Ofast -Wall -Wextra -pedantic -Wno-vla ${CMAKE_CXX_FLAGS}")
project(game-life)


set(SOURCES
    main.cpp)
set(HEADERS
    matrix.h date.h)
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})
target_link_libraries(${PROJECT_NAME} stdc++fs)
