cmake_minimum_required(VERSION 3.10.0)
project(test_sokol VERSION 0.1.0 LANGUAGES C CXX)

set_property(GLOBAL PROPERTY CXX_STANDARD 17)

add_executable(test_sokol main.cpp)

target_include_directories(test_sokol PRIVATE thirdparty)

target_link_libraries(test_sokol X11 Xi Xcursor EGL GL dl pthread m)
