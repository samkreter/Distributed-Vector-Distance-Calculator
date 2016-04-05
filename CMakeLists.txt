cmake_minimum_required (VERSION 2.8)
project (Distributed-Vector-Compare)

set(CMAKE_CXX_FLAGS "--std=c++11 -g -Werror")

set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost 1.45.0 COMPONENTS filesystem thread system)

if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})

    add_executable(main main.cpp)
    target_link_libraries(main ${Boost_LIBRARIES})
endif()

message(AUTHOR_WARNING:"If you see 'Could NOT find Boost' then you must install boost for this to work")



    add_library(saminal src/saminal.cpp)
    target_link_libraries(saminal ${Boost_LIBRARIES})

    add_executable(tester test/unit_tests.cpp)
    target_link_libraries(tester saminal)