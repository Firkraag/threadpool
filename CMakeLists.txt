cmake_minimum_required(VERSION 3.20)
project(threadpool)

set(CMAKE_CXX_STANDARD 17)

list(APPEND objs threadpool.cpp threadpool_lib.cpp)

add_executable(threadpool_test1 threadpool_test1.cpp ${objs})
add_executable(threadpool_test2 threadpool_test2.cpp ${objs})
add_executable(threadpool_test3 threadpool_test3.cpp ${objs})
add_executable(threadpool_test4 threadpool_test4.cpp ${objs})
add_executable(threadpool_test5 threadpool_test5.cpp ${objs})
add_executable(threadpool_test6 threadpool_test6.cpp ${objs})
