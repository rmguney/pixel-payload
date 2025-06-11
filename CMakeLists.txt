cmake_minimum_required(VERSION 3.16)

# Set compiler preference before project() call
if(WIN32 AND NOT DEFINED CMAKE_C_COMPILER)
    find_program(GCC_COMPILER gcc)
    find_program(MINGW_COMPILER mingw32-gcc)
    find_program(MINGW64_COMPILER x86_64-w64-mingw32-gcc)
    
    if(GCC_COMPILER)
        set(CMAKE_C_COMPILER ${GCC_COMPILER})
        message(STATUS "Found and using GCC compiler: ${GCC_COMPILER}")
    elseif(MINGW64_COMPILER)
        set(CMAKE_C_COMPILER ${MINGW64_COMPILER})
        message(STATUS "Found and using MinGW-w64 compiler: ${MINGW64_COMPILER}")
    elseif(MINGW_COMPILER)
        set(CMAKE_C_COMPILER ${MINGW_COMPILER})
        message(STATUS "Found and using MinGW compiler: ${MINGW_COMPILER}")
    else()
        message(STATUS "GCC/MinGW not found, will use default compiler")
    endif()
endif()

project(pxpl VERSION 1.0.0 LANGUAGES C)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Build options
option(BUILD_GUI "Build GUI version" OFF)
option(BUILD_BOTH "Build both CLI and GUI versions" OFF)

# Set output directory to release folder at root
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/release)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/release)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/release)

# Ensure release directory exists
file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/release)

# Include directories
include_directories(include)

# Common source files
set(COMMON_SOURCES
    src/steg.c
    src/image.c
)

# Compiler-specific flags for size optimization
if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -pedantic")
    set(CMAKE_C_FLAGS_RELEASE "-Os -DNDEBUG -ffunction-sections -fdata-sections -fno-unwind-tables -fno-asynchronous-unwind-tables")
    set(CMAKE_C_FLAGS_DEBUG "-O0 -g -DDEBUG")
    
    # Additional linker flags for size optimization
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -Wl,--gc-sections -Wl,--strip-all")
    
elseif(CMAKE_C_COMPILER_ID STREQUAL "MSVC")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W4")
    set(CMAKE_C_FLAGS_RELEASE "/O1 /DNDEBUG /GL")
    set(CMAKE_C_FLAGS_DEBUG "/Od /Zi /DDEBUG")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG /OPT:REF /OPT:ICF")
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

# Windows-specific libraries
if(WIN32)
    set(WINDOWS_LIBS ole32 windowscodecs oleaut32)
    set(GUI_LIBS user32 gdi32 comdlg32)
endif()

# CLI executable (default)
if(NOT BUILD_GUI OR BUILD_BOTH)
    add_executable(pxpl
        src/main.c
        ${COMMON_SOURCES}
    )
    
    target_link_libraries(pxpl ${WINDOWS_LIBS})
    
    # Set properties for CLI version
    if(WIN32 AND CMAKE_C_COMPILER_ID STREQUAL "GNU")
        set_target_properties(pxpl PROPERTIES
            LINK_FLAGS "-mconsole -s"
        )
    endif()
    
    message(STATUS "Building CLI version: pxpl")
endif()

# GUI executable
if(BUILD_GUI OR BUILD_BOTH)
    add_executable(pxpl-gui WIN32
        src/gui.c
        ${COMMON_SOURCES}
    )
    
    target_link_libraries(pxpl-gui ${WINDOWS_LIBS} ${GUI_LIBS})
    
    # Set properties for GUI version
    if(WIN32 AND CMAKE_C_COMPILER_ID STREQUAL "GNU")
        set_target_properties(pxpl-gui PROPERTIES
            LINK_FLAGS "-mwindows -s"
        )
    endif()
    
    message(STATUS "Building GUI version: pxpl-gui")
endif()

# Default build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Print build information
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Compiler: ${CMAKE_C_COMPILER_ID}")
message(STATUS "Output directory: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

# Custom targets for convenience
add_custom_target(test_all
    COMMENT "Run test suite"
    COMMAND ${CMAKE_COMMAND} -E echo "Running test suite..."
    COMMAND cd ${CMAKE_SOURCE_DIR}/tests && dotnet run test
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

if(BUILD_BOTH)
    add_dependencies(test_all pxpl pxpl-gui)
elseif(BUILD_GUI)
    add_dependencies(test_all pxpl-gui)
else()
    add_dependencies(test_all pxpl)
endif()

# Clean target
add_custom_target(clean_all
    COMMENT "Clean all build artifacts and test files"
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_SOURCE_DIR}/build
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_SOURCE_DIR}/release
    COMMAND ${CMAKE_COMMAND} -E remove ${CMAKE_SOURCE_DIR}/tests/bin/Debug/net6.0-windows/*.exe
    COMMAND ${CMAKE_COMMAND} -E remove ${CMAKE_SOURCE_DIR}/tests/bin/Debug/net6.0-windows/*.dll
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# Install targets
if(BUILD_BOTH)
    install(TARGETS pxpl pxpl-gui RUNTIME DESTINATION bin)
elseif(BUILD_GUI)
    install(TARGETS pxpl-gui RUNTIME DESTINATION bin)
else()
    install(TARGETS pxpl RUNTIME DESTINATION bin)
endif()

# Package configuration
set(CPACK_PACKAGE_NAME "pxpl")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "PNG LSB Steganography Tool")
set(CPACK_PACKAGE_VENDOR "pxpl")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/README.md")

include(CPack)
