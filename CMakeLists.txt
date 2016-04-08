cmake_minimum_required (VERSION 2.8)
project (Distributed-Vector-Compare)

set(CMAKE_CXX_FLAGS "--std=c++11 -Werror")

SET(CMAKE_CXX_COMPILER mpicxx)


set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost 1.45.0 COMPONENTS filesystem thread system)

if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})

    add_library(directory src/Directory.cpp)
    add_library(parser src/parser.cpp)
    add_library(timing src/timing.cpp)

    add_executable(main main.cpp)
    target_link_libraries(main ${Boost_LIBRARIES} directory parser timing)
endif()

message(AUTHOR_WARNING:"If you see 'Could NOT find Boost' then you must install boost for this to work")

