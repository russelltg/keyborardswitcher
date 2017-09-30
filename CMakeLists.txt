cmake_minimum_required(VERSION 2.6)
project(keyborardswitcher)

add_executable(keyborardswitcher main.cpp)

install(TARGETS keyborardswitcher RUNTIME DESTINATION bin)
