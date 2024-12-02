cmake_minimum_required(VERSION 3.0...3.25)
project(
	shadyClient
	VERSION 0.1.0
	LANGUAGES CXX C
)

set(EXECUTABLE_NAME shadyClient)  

if (NOT EMSCRIPTEN)
	add_subdirectory(thirdparty/glfw)
else()
	add_library(thirdparty/glfw INTERFACE)
	target_link_options(thirdparty/glfw INTERFACE -sUSE_GLFW=3)
endif()

add_subdirectory(thirdparty/webgpu)
add_subdirectory(thirdparty/glfw3webgpu)


if(WIN32)
    # Create Windows batch script
    file(WRITE ${CMAKE_BINARY_DIR}/kill_previous.bat
        "@echo off\n"
        ":: Find and kill previous instance of the executable\n"
        "taskkill /IM ${EXECUTABLE_NAME}.exe /F >nul 2>&1\n"
        "exit /b 0\n"
    )
    set(KILL_SCRIPT ${CMAKE_BINARY_DIR}/kill_previous.bat)
else()
    # Create Unix shell script
    file(WRITE ${CMAKE_BINARY_DIR}/kill_previous.sh
        "#!/bin/bash\n"
        "# Find and kill previous instance of the executable\n"
        "pkill -f ${EXECUTABLE_NAME} || true\n"
       
    )
    
    # Make the script executable on Unix
    execute_process(
        COMMAND chmod +x ${CMAKE_BINARY_DIR}/kill_previous.sh
    )
    set(KILL_SCRIPT ${CMAKE_BINARY_DIR}/kill_previous.sh)
endif()

add_custom_target(kill_previous
    COMMAND ${KILL_SCRIPT}
    COMMENT "Killing previous instance of ${EXECUTABLE_NAME}"
)

file(GLOB_RECURSE SOURCES 
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cppm"
)

add_executable(shadyClient "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")

target_sources(shadyClient
  PUBLIC
    FILE_SET CXX_MODULES FILES
      ${SOURCES})

add_dependencies(shadyClient kill_previous)

find_package(glm CONFIG REQUIRED)

find_package(nlohmann_json CONFIG REQUIRED)

target_link_libraries(shadyClient PRIVATE nlohmann_json::nlohmann_json)

target_link_libraries(shadyClient PRIVATE glm::glm-header-only glfw webgpu glfw3webgpu)

target_copy_webgpu_binaries(shadyClient)

set_target_properties(shadyClient PROPERTIES
	CXX_STANDARD 20
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

add_custom_command(TARGET shadyClient POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${PROJECT_SOURCE_DIR}/resources"
        $<TARGET_FILE_DIR:shadyClient>/resources)
