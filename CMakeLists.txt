cmake_minimum_required(VERSION 3.16)

# Use MSVC for optimal binary size (25KB GUI, 20KB CLI)
if(WIN32 AND NOT DEFINED CMAKE_C_COMPILER)
    message(STATUS "Using default MSVC compiler for minimal binary size")
endif()

project(pxpl VERSION 1.0.0 LANGUAGES C)

# Windows-only optimized build (cross-platform support planned for future)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

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

# Compiler flags optimized for MSVC minimal size and stealth
set(CMAKE_C_FLAGS "/W4 /TC")
set(CMAKE_C_FLAGS_RELEASE "/O1 /DNDEBUG /GL /Gy /GS- /Gm- /fp:fast")
set(CMAKE_C_FLAGS_DEBUG "/Od /Zi /DDEBUG")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "/LTCG /OPT:REF /OPT:ICF /MERGE:.rdata=.text /MERGE:.pdata=.text")
add_definitions(-D_CRT_SECURE_NO_WARNINGS -DWIN32_LEAN_AND_MEAN)

# Windows libraries
set(WINDOWS_LIBS ole32 windowscodecs oleaut32)
set(GUI_LIBS user32 gdi32 comdlg32)

# Build both CLI and GUI executables
add_executable(pxpl
    src/main.c
    ${COMMON_SOURCES}
)

add_executable(pxpl-gui WIN32
    src/gui.c
    ${COMMON_SOURCES}
)

target_link_libraries(pxpl ${WINDOWS_LIBS})
target_link_libraries(pxpl-gui ${WINDOWS_LIBS} ${GUI_LIBS})

message(STATUS "Building both CLI and GUI versions: pxpl, pxpl-gui")

# Default build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Print build information
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Compiler: ${CMAKE_C_COMPILER_ID}")
message(STATUS "Output directory: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

# Clean target
add_custom_target(clean_all
    COMMENT "Clean all build artifacts and test files"
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_SOURCE_DIR}/build
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_SOURCE_DIR}/release
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# Install targets
install(TARGETS pxpl pxpl-gui RUNTIME DESTINATION bin)

# Package configuration
set(CPACK_PACKAGE_NAME "pxpl")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "pxpl")
set(CPACK_PACKAGE_VENDOR "pxpl")
set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/README.md")

include(CPack)
