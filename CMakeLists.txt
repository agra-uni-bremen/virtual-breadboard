cmake_minimum_required(VERSION 3.5.0)
project(vp-breadboard-gui VERSION 2.0.0 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
cmake_policy(SET CMP0071 NEW)	# not required, will be standard in some time

find_package(Qt5Widgets CONFIG REQUIRED)

find_library(LUA_LIB lua)
if(NOT LUA_LIB)
    find_library(LUA_LIB lua5.3)
    if(NOT LUA_LIB)
      message(FATAL_ERROR "lua library not found")
    endif()
endif()

if(NOT DONT_INCLUDE_LUA_DEVICES)
    set(DONT_INCLUDE_LUA_DEVICES false)
endif()
if(NOT DONT_INCLUDE_CONFIGS)
    set(DONT_INCLUDE_CONFIGS false)
endif()

include(CMake/AddGitSubmodule.cmake)
include(CMake/GenerateResourceFiles.cmake)

add_git_submodule(lib/LuaBridge3)
add_git_submodule(lib/protocol)


if(NOT DONT_INCLUDE_CONFIGS)
	file(GLOB CONFIGS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ./conf/*.json)
endif()
if(NOT DONT_INCLUDE_LUA_DEVICES)
	file(GLOB SCRIPTS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ./devices/lua/*.lua)
endif()
set(SCRIPTS ${SCRIPTS} src/device/factory/loadscript.lua)
file(GLOB IMAGES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "./img/*.jpg" "./img/*.jpeg" "./img/*.png")

add_subdirectory(src/device)
add_subdirectory(src/breadboard)
add_subdirectory(src/embedded)
add_subdirectory(src/window)
add_subdirectory(devices)

add_executable(vp-breadboard src/main.cpp)
target_compile_features(vp-breadboard PUBLIC cxx_std_17)
target_link_libraries(vp-breadboard window Qt5::Widgets -Wl,--whole-archive c-devices -Wl,--no-whole-archive)
set_target_properties(vp-breadboard PROPERTIES
	AUTOMOC ON
	AUTOUIC ON
	AUTORCC ON
)
add_qt_resource(vp-breadboard configs FILES ${CONFIGS})
add_qt_resource(vp-breadboard scripts FILES ${SCRIPTS})
add_qt_resource(vp-breadboard images FILES ${IMAGES})

#target_link_libraries(vp-breadboard Qt5::Widgets pthread LuaBridge ${LUA_LIB} virtual-breadboard-client)

