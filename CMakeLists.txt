cmake_minimum_required(VERSION 3.15)
project(tora)
include(build/kungfu.cmake)
kungfu_setup(tora)

add_compile_definitions(SPDLOG_ACTIVE_LEVEL=0)
add_compile_definitions(SPDLOG_NO_NAME)
add_compile_definitions(SPDLOG_NO_ATOMIC_LEVELS)