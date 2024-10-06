cmake_minimum_required(VERSION 3.0...3.25)
project(
	LearnWebGPU
	VERSION 0.1.0
	LANGUAGES CXX C
)

if (NOT EMSCRIPTEN)
	add_subdirectory(glfw)
else()
	add_library(glfw INTERFACE)
	target_link_options(glfw INTERFACE -sUSE_GLFW=3)
endif()

add_subdirectory(webgpu)
add_subdirectory(glfw3webgpu)

add_executable(App
	main.cpp
)

target_link_libraries(App PRIVATE glfw webgpu glfw3webgpu)

target_copy_webgpu_binaries(App)


file(GLOB_RECURSE SHADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.wgsl")

# foreach(SHADER_FILE ${SHADER_FILES})
# 	string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}/shaders/" "" RELATIVE_PATH ${SHADER_FILE})
# 	file(READ ${SHADER_FILE} content)
# 	set(content "R\"(\n${content})\"")
# 	file(WRITE "${CMAKE_CURRENT_SOURCE_DIR}/generated/${RELATIVE_PATH}" "${content}")
# endforeach()


set_target_properties(App PROPERTIES
	CXX_STANDARD 17
	CXX_STANDARD_REQUIRED ON
	CXX_EXTENSIONS OFF
	COMPILE_WARNING_AS_ERROR ON
)

if (MSVC)
	target_compile_options(App PRIVATE /W4)
else()
	target_compile_options(App PRIVATE -Wall -Wextra -pedantic)
endif()

if (XCODE)
	set_target_properties(App PROPERTIES
		XCODE_GENERATE_SCHEME ON
		XCODE_SCHEME_ENABLE_GPU_FRAME_CAPTURE_MODE "Metal"
	)
endif()

if (EMSCRIPTEN)
	set_target_properties(App PROPERTIES SUFFIX ".html")
	target_link_options(App PRIVATE -sASYNCIFY)
endif()
