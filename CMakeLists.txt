cmake_minimum_required(VERSION 2.6)
project(keyboardswitcher)

add_executable(keyboardswitcher main.cpp)

install(TARGETS keyboardswitcher RUNTIME DESTINATION bin)

target_link_libraries(keyboardswitcher PUBLIC xcb xcb-xinput)
target_compile_options(keyboardswitcher PUBLIC -std=c++14)
