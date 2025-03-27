#  _               _ 
# | |__   _____  _(_)
# | '_ \ / _ \ \/ / | MIT & Apache 2.0 dual licensed
# | | | |  __/>  <| | Version 1.0
# |_| |_|\___/_/\_\_| https://github.com/EmberEmu/hexi

cmake_minimum_required(VERSION 3.29)
set(HEXI_PROJECT_NAME hexi)
project(${HEXI_PROJECT_NAME})

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED OFF)

option(ENABLE_TESTING OFF)

##############################
#         Google Test        #
##############################
if(ENABLE_TESTING)
	find_package(Threads REQUIRED)
    include(FetchContent)

    FetchContent_Declare(
      googletest
      URL https://github.com/google/googletest/releases/download/v1.16.0/googletest-1.16.0.tar.gz
      URL_HASH SHA512=bec8dad2a5abbea8e9e5f0ceedd8c9dbdb8939e9f74785476b0948f21f5db5901018157e78387e106c6717326558d6642fc0e39379c62af57bf1205a9df8a18b 
    )

    # For Windows: Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
    set(FETCHCONTENT_FULLY_DISCONNECTED ON)

    include(GoogleTest)

    set(cmake_ctest_arguments "CTEST_OUTPUT_ON_FAILURE")
    enable_testing()
    add_subdirectory(tests)
endif()

add_subdirectory(include)
