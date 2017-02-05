# Copyright 2017 Jeremy Letang.
# Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
# http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
# <LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
# option. This file may not be copied, modified, or distributed
# except according to those terms.

cmake_minimum_required (VERSION 2.8.1)

project (kiwi_example C)

set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu11 -W -Wall -Wextra")

# define custom cmake module path
# here it goes to parent cmake folder which is at the root
# of the kiwi repository
set (CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/../cmake)

# include kiwi module
include (kiwi)

# set the kiwi parameters
# first is the output folder for kiwi generated files
# second is the list of type / include files
kiwi_build(
  "${CMAKE_SOURCE_DIR}/kiwi_gen"
  "int" "char" "str:char *"
  )

# add kiwi include dir to the project include directories
include_directories (${KIWI_INCLUDE_DIR})

# create our executable adding our sources + kiwi sources
add_executable(kiwi_example
  src/main.c
  ${KIWI_SOURCES}
  )

# add kiwi as a dependency of our executable to ensure
# kiwi to be built first and run before our main program
add_dependencies(kiwi_example kiwi)
