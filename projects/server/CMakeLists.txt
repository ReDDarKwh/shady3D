cmake_minimum_required(VERSION 3.0...3.25)
project(
	shadyServer
	VERSION 0.1.0
	LANGUAGES CXX C
)

add_subdirectory(thirdparty/efsw)

add_executable(shadyServer
main.cpp 
)

target_link_libraries(shadyServer PRIVATE asio efsw)

set_target_properties(shadyClient PROPERTIES
	CXX_STANDARD 17
	CXX_STANDARD_REQUIRED ON
	CXX_EXTENSIONS OFF
	COMPILE_WARNING_AS_ERROR ON
)