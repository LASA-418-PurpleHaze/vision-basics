
cmake_minimum_required(VERSION 3.2)
project(master_template)
set(CMAKE_CXX_STANDARD 11)
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -lpthread")

find_package( OpenCV REQUIRED )
find_package(PkgConfig)
find_package(Threads)
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
add_executable(master_template master_template.cpp )
target_link_libraries( master_template ${OpenCV_LIBS}
				   ${ZeroMQ_LIBRARY} ${CMAKE_THREAD_LIBS_INIT}) 
