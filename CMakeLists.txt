# Copyright 2016 Jeremy Letang.
# Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
# http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
# <LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
# option. This file may not be copied, modified, or distributed
# except according to those terms.

cmake_minimum_required (VERSION 2.8.1)

project (kiwi C)

set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu11 -W -Wall -Wextra")
set (CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g")
set (CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O2")

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

file (GLOB SOURCES src/*.c)

add_executable(kiwi ${SOURCES})
