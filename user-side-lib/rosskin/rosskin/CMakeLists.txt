cmake_minimum_required(VERSION 2.4.6)
include($ENV{ROS_ROOT}/core/rosbuild/rosbuild.cmake)

# Set the build type.  Options are:
#  Coverage       : w/ debug symbols, w/o optimization, w/ code-coverage
#  Debug          : w/ debug symbols, w/o optimization
#  Release        : w/o debug symbols, w/ optimization
#  RelWithDebInfo : w/ debug symbols, w/ optimization
#  MinSizeRel     : w/o debug symbols, w/ optimization, stripped binaries
#set(ROS_BUILD_TYPE RelWithDebInfo)

rosbuild_init()

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

rosbuild_genmsg()
rosbuild_gensrv()

include(config.cmake)
SET(SKIN_LIB ${SKIN_TOP}/user-side-lib)
SET(SKIN_KERNEL ${SKIN_TOP}/kernel-module)
include_directories(include ${SKIN_LIB} ${SKIN_KERNEL})
link_directories(${SKIN_LIB})

# ROSSkin server:
if (NOT ${ROSSKIN_LIB_ONLY})
  rosbuild_add_executable(rosskin-server src/server.cpp)
  target_link_libraries(rosskin-server ${PROJECT_NAME} skin)
  include(extra.cmake)
endif()

# ROSSkin library:
rosbuild_add_library(${PROJECT_NAME} src/user.cpp ${SKIN_LIB}/skin_sensor_type.c ${SKIN_LIB}/skin_sensor.c
	${SKIN_LIB}/skin_module.c ${SKIN_LIB}/skin_patch.c ${SKIN_LIB}/skin_sub_region.c ${SKIN_LIB}/skin_region.c
	${SKIN_LIB}/skin_object.c ${SKIN_LIB}/skin_log.c ${SKIN_LIB}/skin_iterators.cpp)
set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_DEFINITIONS "SKIN_DS_ONLY")
set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "-Wall")

# Tests:
rosbuild_add_executable(rosskin-usertest-sporadic test/sporadic.cpp)
set_target_properties(rosskin-usertest-sporadic PROPERTIES COMPILE_DEFINITIONS "SKIN_DS_ONLY")
target_link_libraries(rosskin-usertest-sporadic ${PROJECT_NAME})
rosbuild_add_executable(rosskin-usertest-periodic test/periodic.cpp)
set_target_properties(rosskin-usertest-periodic PROPERTIES COMPILE_DEFINITIONS "SKIN_DS_ONLY")
target_link_libraries(rosskin-usertest-periodic ${PROJECT_NAME})
