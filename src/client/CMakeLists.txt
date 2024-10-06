cmake_minimum_required(VERSION 3.0...3.25)
project(
	shadyClient
	VERSION 0.1.0
	LANGUAGES CXX C
)

if (NOT EMSCRIPTEN)
	add_subdirectory(thirdparty/glfw)
else()
	add_library(thirdparty/glfw INTERFACE)
	target_link_options(thirdparty/glfw INTERFACE -sUSE_GLFW=3)
endif()

add_subdirectory(thirdparty/webgpu)
add_subdirectory(thirdparty/glfw3webgpu)

add_executable(shadyClient
	main.cpp
)

target_link_libraries(shadyClient PRIVATE glfw webgpu glfw3webgpu)

target_copy_webgpu_binaries(shadyClient)

set_target_properties(shadyClient PROPERTIES
	CXX_STANDARD 17
	CXX_STANDARD_REQUIRED ON
	CXX_EXTENSIONS OFF
	COMPILE_WARNING_AS_ERROR ON
)

if (MSVC)
	target_compile_options(shadyClient PRIVATE /W4)
else()
	target_compile_options(shadyClient PRIVATE -Wall -Wextra -pedantic)
endif()

if (XCODE)
	set_target_properties(shadyClient PROPERTIES
		XCODE_GENERATE_SCHEME ON
		XCODE_SCHEME_ENABLE_GPU_FRAME_CAPTURE_MODE "Metal"
	)
endif()

if (EMSCRIPTEN)
	set_target_properties(shadyClient PROPERTIES SUFFIX ".html")
	target_link_options(shadyClient PRIVATE -sASYNCIFY)
endif()
