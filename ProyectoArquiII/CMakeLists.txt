cmake_minimum_required(VERSION 3.31)
project(ProyectoArquiII)

set(CMAKE_CXX_STANDARD 20)

add_executable(ProyectoArquiII main.cpp
        PE.h
        PE.cpp
        Interconnect.h
        Interconnect.cpp
        Message.h)
