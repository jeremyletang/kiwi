# Copyright 2017 Jeremy Letang.
# Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
# http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
# <LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
# option. This file may not be copied, modified, or distributed
# except according to those terms.

cmake_minimum_required(VERSION 2.8.8)
include(ExternalProject)

message(STATUS "Configuring kiwi")

set (KIWI_DIR kiwi)
set (KIWI_PATH ${CMAKE_BINARY_DIR}/${KIWI_DIR})

ExternalProject_Add(
  deps.kiwi
  PREFIX ${KIWI_PATH}
  GIT_REPOSITORY https://github.com/jeremyletang/kiwi.git
  TIMEOUT 10
  CONFIGURE_COMMAND ${CMAKE_COMMAND} "-DCMAKE_INSTALL_PREFIX=${KIWI_PATH}" <SOURCE_DIR>
  BUILD_IN_SOURCE ON
  UPDATE_COMMAND ""
  BUILD_COMMAND ${CMAKE_MAKE_PROGRAM}
  INSTALL_COMMAND ""
  LOG_DOWNLOAD ON
  LOG_UPDATE ON
  LOG_CONFIGURE ON
  LOG_BUILD ON
  )

ExternalProject_Get_Property(deps.kiwi SOURCE_DIR)
set (KIWI_CMD "${SOURCE_DIR}/bin/kiwi")

macro(kiwi_build out)
  file(MAKE_DIRECTORY ${out})
  set (KIWI_INCLUDE_DIR "${out}")
  set (KIWI_SOURCES "${out}/kiwi.c")
  add_custom_command(
    OUTPUT "${KIWI_INCLUDE_DIR}/kiwi.h" "${KIWI_SOURCES}"
    COMMAND ${KIWI_CMD} "--out=${out}" ${ARGN}
    DEPENDS ${KIWI_CMD}
    COMMENT "generating kiwi c code"
    VERBATIM)
endmacro()
