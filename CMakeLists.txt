cmake_minimum_required(VERSION 3.25)

project(TaikoArcadeLoader VERSION 1.0.0 LANGUAGES C CXX)

# Set runtime library to static
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE INTERNAL "")
endif()

find_package(Git REQUIRED)

# Commands to read each needed variable
set(variablesToRead "GIT_BRANCH;GIT_SHA1;GIT_SHORTSHA1;GIT_DIRTY")
set(CMD_GIT_VERSION     ${GIT_EXECUTABLE} describe --tags --dirty)

execute_process(
    COMMAND ${CMD_GIT_VERSION}
    RESULT_VARIABLE GIT_VERSION_RESULT
    OUTPUT_VARIABLE GIT_VERSION
    ERROR_VARIABLE GIT_ERROR
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Set C and C++ standards
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set warning level
if(MSVC)
    add_compile_options(/W3)
    # Set the source file encoding to UTF-8
    add_compile_options(/utf-8)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Add project definitions (Add ASYNC_UPDATE/ASYNC_IO to enable new async feature [experimental])
# add_definitions(-DNOMINMAX -DLTC_NO_PROTOTYPES -D_CRT_SECURE_NO_WARNINGS -DASYNC_UPDATE)
add_definitions(-DNOMINMAX -DLTC_NO_PROTOTYPES -D_CRT_SECURE_NO_WARNINGS -DTAL_VERSION="${GIT_VERSION}")
MESSAGE( STATUS "Build Version = ${GIT_VERSION}.")


# Include FetchContent module
include(FetchContent)

# Fetch stb (header-only library, no built-in CMake support)
FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG master
)
FetchContent_GetProperties(stb)
if(NOT stb_POPULATED)
    FetchContent_MakeAvailable(stb)
    add_library(stb INTERFACE)
    target_include_directories(stb INTERFACE ${stb_SOURCE_DIR})
endif()

# Fetch zxing-cpp
FetchContent_Declare(
    zxing_cpp
    URL https://github.com/zxing-cpp/zxing-cpp/archive/refs/tags/v2.2.1.zip
)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(zxing_cpp)

# Fetch zlib
FetchContent_Declare(
    zlib
    GIT_REPOSITORY https://github.com/madler/zlib.git
    GIT_TAG 51b7f2abdade71cd9bb0e7a373ef2610ec6f9daf
)
FetchContent_MakeAvailable(zlib)

# Fetch libtomcrypt
FetchContent_Declare(
    libtomcrypt
    GIT_REPOSITORY https://github.com/libtom/libtomcrypt.git
    GIT_TAG 124e020437715b0d2647ed12632fa10e2cfe9234 # v1.18.2 does not have cmake
)
set(BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(WITH_LTM OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(libtomcrypt)

# Fetch pugixml
FetchContent_Declare(
    pugixml
    URL https://github.com/zeux/pugixml/archive/v1.14.tar.gz
)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(PUGIXML_WCHAR_MODE ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(pugixml)

# Fetch safetyhook
FetchContent_Declare(
    safetyhook
    GIT_REPOSITORY https://github.com/cursey/safetyhook.git
    GIT_TAG v0.4.1
)
set(SAFETYHOOK_FETCH_ZYDIS ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(safetyhook)

# Fetch SDL3
FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG 78cc5c173404488d80751af226d1eaf67033bcc4 # v3.1.6
    # URL https://github.com/libsdl-org/SDL/releases/download/preview-3.1.6/SDL3-3.1.6.tar.gz
)
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TEST OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(SDL3)

# Fetch xxHash
FetchContent_Declare(
    xxhash
    URL https://github.com/Cyan4973/xxHash/archive/v0.8.2.tar.gz
    SOURCE_SUBDIR cmake_unofficial
)
set(XXH_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(XXH_BUILD_STATIC ON CACHE BOOL "" FORCE)
set(XXH_BUILD_XXHSUM OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(xxhash)

# Fetch Minhook
FetchContent_Declare(
    minhook
    GIT_REPOSITORY https://github.com/TsudaKageyu/minhook.git
    GIT_TAG d862245d98fecd56dd7b1ca2f2e5185b75ecc780
)
FetchContent_MakeAvailable(minhook)

FetchContent_Declare(
        tomlplusplus
        GIT_REPOSITORY https://github.com/marzer/tomlplusplus.git
        GIT_TAG v3.4.0
)
FetchContent_MakeAvailable(tomlplusplus)
export(TARGETS tomlplusplus_tomlplusplus  FILE "${CMAKE_BINARY_DIR}/tomlplusplus-config.cmake")
set(tomlplusplus_DIR "${CMAKE_BINARY_DIR}")

FetchContent_Declare(
        reflectcpp
        URL https://github.com/getml/reflect-cpp/archive/refs/tags/v0.17.0.zip
)
set(REFLECTCPP_TOML ON CACHE BOOL "" FORCE)
set(REFLECTCPP_XML OFF CACHE BOOL "" FORCE)
set(REFLECTCPP_USE_VCPKG OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(reflectcpp)

# Source files
set(SOURCES
        src/dllmain.cpp
        src/helpers.cpp
        src/logger.cpp
        src/poll.cpp
        src/pollasync.cpp
        src/bnusio.cpp
        src/Windows/WindowsError.cpp
        src/Windows/MinimumLatencyAudioClient.cpp
        src/patches/amauth.cpp
        # src/patches/dxgi.cpp
        # src/patches/fpslimiter.cpp
        src/patches/audio.cpp
        src/patches/plugins.cpp
        src/patches/scanner.cpp
        src/patches/layeredfs.cpp
        src/patches/testmode.cpp
        src/patches/timer.cpp
        src/patches/language.cpp
        # src/patches/unlimitsong.cpp
        src/patches/versions/JPN00.cpp
        src/patches/versions/JPN08.cpp
        src/patches/versions/JPN39.cpp
        src/patches/versions/CHN00.cpp
        src/config.cpp
        src/banner.cpp
)

set(VALIDATOR_SOURCES
        src/configValidator.cpp
        src/helpers.cpp
        src/config.cpp
        src/logger.cpp
)

# Create the library
add_library(bnusio SHARED ${SOURCES})
add_executable(validator ${VALIDATOR_SOURCES})

# Remove 'lib' prefix from the library name, make sure the output directory is expected
set_target_properties(bnusio PROPERTIES
        PREFIX ""
	LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>")

set_target_properties(validator PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>"
)


# Include directories
target_include_directories(bnusio PRIVATE
        src
        ${SDL3_SOURCE_DIR}/include
        ${xxhash_SOURCE_DIR}
        ${zlib_SOURCE_DIR}
        ${libtomcrypt_SOURCE_DIR}/src/headers
        ${minhook_SOURCE_DIR}/include
        ${reflectcpp_SOURCE_DIR}/include
)

target_include_directories(validator PRIVATE
        src
        ${SDL3_SOURCE_DIR}/include
        ${xxhash_SOURCE_DIR}
        ${zlib_SOURCE_DIR}
        ${libtomcrypt_SOURCE_DIR}/src/headers
        ${minhook_SOURCE_DIR}/include
        ${reflectcpp_SOURCE_DIR}/include
)

# Compiler definitions
target_compile_definitions(bnusio PRIVATE
    _WIN32_WINNT=_WIN32_WINNT_WIN10
)

# Add link options
if(NOT MSVC)
    target_link_options(bnusio PRIVATE -Wl,--allow-multiple-definition)
endif()

# Link libraries
target_link_libraries(bnusio PRIVATE
        SDL3-static
        xxhash
        zlibstatic
        libtomcrypt
        safetyhook
        ZXing::ZXing
        pugixml
        stb
        ws2_32
        ntdll
        minhook
        tomlplusplus::tomlplusplus
        reflectcpp
)

target_link_libraries(validator PRIVATE
        SDL3-static
        xxhash
        zlibstatic
        libtomcrypt
        safetyhook
        ZXing::ZXing
        pugixml
        stb
        ws2_32
        ntdll
        minhook
        tomlplusplus::tomlplusplus
        reflectcpp
)

# Define log path; used to make the file path relative in the log calls.
# Last character (-) to remove the trailing slash in the log path
add_compile_definitions("SOURCE_ROOT=${CMAKE_CURRENT_SOURCE_DIR}/src-")

# Set runtime library to static
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# Define the .def file
set(DEF_FILE "${CMAKE_CURRENT_SOURCE_DIR}/exports.def")

if (MSVC)
    # Add the .def file to the target properties
    set_target_properties(bnusio PROPERTIES LINK_FLAGS "/DEF:${DEF_FILE}")
endif()

add_custom_command(
    TARGET bnusio POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_BINARY_DIR}/$<CONFIG>/bnusio.dll"
        "${CMAKE_SOURCE_DIR}/dist/bnusio.dll"
    COMMENT "Copying bnusio.dll to dist directory"
)

# Set a default target
add_custom_target(default ALL DEPENDS bnusio)