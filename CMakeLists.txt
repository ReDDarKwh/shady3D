cmake_minimum_required(VERSION 3.0...3.25)
project(
	shadyClient
	VERSION 0.1.0
	LANGUAGES CXX C
)

set(EXECUTABLE_NAME shadyClient)  

include(cmake/CPM.cmake)

find_package(Threads REQUIRED)

CPMAddPackage("gh:chriskohlhoff/asio#asio-1-30-2@1.30.2")

# ASIO doesn't use CMake, we have to configure it manually. Extra notes for using on Windows:
#
# 1) If _WIN32_WINNT is not set, ASIO assumes _WIN32_WINNT=0x0501, i.e. Windows XP target, which is
# definitely not the platform which most users target.
#
# 2) WIN32_LEAN_AND_MEAN is defined to make Winsock2 work.
if(asio_ADDED)
  add_library(asio INTERFACE)

  target_include_directories(asio SYSTEM INTERFACE ${asio_SOURCE_DIR}/asio/include)

  target_compile_definitions(asio INTERFACE ASIO_STANDALONE ASIO_NO_DEPRECATED)

  target_link_libraries(asio INTERFACE Threads::Threads)

  if(WIN32)
    # macro see @ https://stackoverflow.com/a/40217291/1746503
    macro(get_win32_winnt version)
      if(CMAKE_SYSTEM_VERSION)
        set(ver ${CMAKE_SYSTEM_VERSION})
        string(REGEX MATCH "^([0-9]+).([0-9])" ver ${ver})
        string(REGEX MATCH "^([0-9]+)" verMajor ${ver})
        # Check for Windows 10, b/c we'll need to convert to hex 'A'.
        if("${verMajor}" MATCHES "10")
          set(verMajor "A")
          string(REGEX REPLACE "^([0-9]+)" ${verMajor} ver ${ver})
        endif("${verMajor}" MATCHES "10")
        # Remove all remaining '.' characters.
        string(REPLACE "." "" ver ${ver})
        # Prepend each digit with a zero.
        string(REGEX REPLACE "([0-9A-Z])" "0\\1" ver ${ver})
        set(${version} "0x${ver}")
      endif()
    endmacro()

    if(NOT DEFINED _WIN32_WINNT)
      get_win32_winnt(ver)
      set(_WIN32_WINNT ${ver})
    endif()

    message(STATUS "Set _WIN32_WINNET=${_WIN32_WINNT}")

    target_compile_definitions(asio INTERFACE _WIN32_WINNT=${_WIN32_WINNT} WIN32_LEAN_AND_MEAN)
  endif()
endif()

if (NOT EMSCRIPTEN)
	add_subdirectory(thirdparty/glfw)
else()
	add_library(thirdparty/glfw INTERFACE)
	target_link_options(thirdparty/glfw INTERFACE -sUSE_GLFW=3)
endif()

add_subdirectory(thirdparty/webgpu)
add_subdirectory(thirdparty/glfw3webgpu)

file(GLOB_RECURSE SOURCES 
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

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

add_executable(shadyClient ${SOURCES})

add_dependencies(shadyClient kill_previous)

find_package(glm CONFIG REQUIRED)

target_link_libraries(shadyClient PRIVATE glm::glm-header-only glfw webgpu glfw3webgpu asio)

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

add_custom_command(TARGET shadyClient POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${PROJECT_SOURCE_DIR}/resources"
        $<TARGET_FILE_DIR:shadyClient>/resources)
