cmake_minimum_required(VERSION 3.14)
project(csopesy-fcfs CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(Threads REQUIRED)

add_executable(csopesy-fcfs
    src/main.cpp
    src/Config.cpp
    src/TimeUtil.cpp
    src/Process.cpp
    src/PrintLogger.cpp
    src/Scheduler.cpp
    src/Console.cpp
)
target_include_directories(csopesy-fcfs PRIVATE include)
target_link_libraries(csopesy-fcfs PRIVATE Threads::Threads)
