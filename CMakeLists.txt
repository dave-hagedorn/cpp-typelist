cmake_minimum_required(VERSION 3.1)

project (dhagedorn-types VERSION 0.1.0 LANGUAGES CXX)

add_library(dhagedorn-types INTERFACE)

target_compile_features(dhagedorn-types INTERFACE cxx_std_20)

target_include_directories(dhagedorn-types INTERFACE include)

add_subdirectory(sample)