cmake_minimum_required(VERSION 3.15)

project(RISC-V-Simulator)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

include_directories(src)

add_executable(code main.cpp)


# enable_testing()
# file(GLOB testcases RELATIVE ${PROJECT_SOURCE_DIR}/data data/*.data)
# foreach (testcase ${testcases})
#     string(REPLACE ".data" "" testcase ${testcase})
#     if (testcase STREQUAL "pi")
#         continue()
#     endif()
#     add_test(NAME ${testcase} WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/run
#             COMMAND bash -c "$<TARGET_FILE:code> <../data/${testcase}.data")
#     set_property(TEST ${testcase} PROPERTY TIMEOUT 60)
# endforeach ()