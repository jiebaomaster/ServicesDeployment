cmake_minimum_required(VERSION 3.21)
project(task_deployment)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)

add_executable(task_deployment
        experiment.cpp
#        test.cpp
        )
