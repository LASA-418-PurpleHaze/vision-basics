
cmake_minimum_required(VERSION 3.2)
project(jetson_sim)
set(CMAKE_CXX_STANDARD 14)
find_package( OpenCV REQUIRED )
find_package(PkgConfig)
pkg_check_modules(PC_ZeroMQ QUIET zmq)
find_path(ZeroMQ_INCLUDE_DIR
	NAMES zmq.hpp
	PATHS ${PC_ZeroMQ_INCLUDE_DIRS}
	)
find_library(ZeroMQ_LIBRARY
	NAMES zmq
	PATHS ${PC_ZeroMQ_LIBRARY_DIRS}
	)
set(SOURCE_FILES main.cpp)
include_directories( ${OpenCV_INCLUDE_DIRS} )
add_executable(jetson_sim jetson_sim.cpp )
target_link_libraries( jetson_sim ${OpenCV_LIBS}
				   ${ZeroMQ_LIBRARY} )
