cmake_minimum_required(VERSION 3.1)

add_executable(sample sample.cc)

target_include_directories(sample PRIVATE ${CMAKE_BINARY_DIR}/include)

target_link_libraries(sample PRIVATE dhagedorn-types)