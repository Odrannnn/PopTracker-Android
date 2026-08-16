cmake_minimum_required(VERSION 3.22.1)
project(poptracker_android LANGUAGES C CXX)

get_filename_component(POPTRACKER_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../../../" ABSOLUTE)
set(SDL_ROOT "${POPTRACKER_ROOT}/android/vendor/SDL")
set(SDL_IMAGE_ROOT "${POPTRACKER_ROOT}/android/vendor/SDL_image")
set(SDL_TTF_ROOT "${POPTRACKER_ROOT}/android/vendor/SDL_ttf")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Build the SDL family as shared libraries loaded by PopTrackerActivity.
set(SDL_SHARED ON CACHE BOOL "" FORCE)
set(SDL_STATIC OFF CACHE BOOL "" FORCE)
set(SDL_TEST OFF CACHE BOOL "" FORCE)
set(SDL_TESTS OFF CACHE BOOL "" FORCE)
set(SDL_DISABLE_INSTALL ON CACHE BOOL "" FORCE)
add_subdirectory("${SDL_ROOT}" "${CMAKE_CURRENT_BINARY_DIR}/SDL" EXCLUDE_FROM_ALL)

# Tracker packs officially use PNG, JPEG and GIF. stb_image handles PNG/JPEG,
# and SDL_image contains its own GIF loader, avoiding a large dependency tree.
set(BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)
set(SDL2IMAGE_INSTALL OFF CACHE BOOL "" FORCE)
set(SDL2IMAGE_SAMPLES OFF CACHE BOOL "" FORCE)
set(SDL2IMAGE_TESTS OFF CACHE BOOL "" FORCE)
set(SDL2IMAGE_BACKEND_STB ON CACHE BOOL "" FORCE)
set(SDL2IMAGE_AVIF OFF CACHE BOOL "" FORCE)
set(SDL2IMAGE_JXL OFF CACHE BOOL "" FORCE)
set(SDL2IMAGE_TIF OFF CACHE BOOL "" FORCE)
set(SDL2IMAGE_WEBP OFF CACHE BOOL "" FORCE)
set(SDL2IMAGE_SVG OFF CACHE BOOL "" FORCE)
set(SDL2IMAGE_PNG_SAVE OFF CACHE BOOL "" FORCE)
set(SDL2IMAGE_JPG_SAVE OFF CACHE BOOL "" FORCE)
add_subdirectory("${SDL_IMAGE_ROOT}" "${CMAKE_CURRENT_BINARY_DIR}/SDL_image" EXCLUDE_FROM_ALL)

set(SDL2TTF_INSTALL OFF CACHE BOOL "" FORCE)
set(SDL2TTF_SAMPLES OFF CACHE BOOL "" FORCE)
set(SDL2TTF_VENDORED ON CACHE BOOL "" FORCE)
set(SDL2TTF_HARFBUZZ OFF CACHE BOOL "" FORCE)
set(FT_DISABLE_BZIP2 ON CACHE BOOL "" FORCE)
set(FT_DISABLE_BROTLI ON CACHE BOOL "" FORCE)
set(FT_DISABLE_HARFBUZZ ON CACHE BOOL "" FORCE)
set(FT_DISABLE_PNG ON CACHE BOOL "" FORCE)
set(FT_DISABLE_ZLIB ON CACHE BOOL "" FORCE)
add_subdirectory("${SDL_TTF_ROOT}" "${CMAKE_CURRENT_BINARY_DIR}/SDL_ttf" EXCLUDE_FROM_ALL)

# PopTracker uses the installed SDL2 header layout (SDL2/SDL_image.h and
# SDL2/SDL_ttf.h), while the dependency source trees expose those headers at
# their include roots. Provide the installed layout without modifying the
# vendored submodules.
set(SDL2_COMPAT_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/sdl2-compat-include")
file(MAKE_DIRECTORY "${SDL2_COMPAT_INCLUDE_DIR}/SDL2")
file(GENERATE
    OUTPUT "${SDL2_COMPAT_INCLUDE_DIR}/SDL2/SDL_image.h"
    CONTENT "#include <SDL_image.h>\n"
)
file(GENERATE
    OUTPUT "${SDL2_COMPAT_INCLUDE_DIR}/SDL2/SDL_ttf.h"
    CONTENT "#include <SDL_ttf.h>\n"
)

find_package(openssl REQUIRED CONFIG)

file(GLOB POPTRACKER_SOURCES CONFIGURE_DEPENDS
    "${POPTRACKER_ROOT}/src/*.cpp"
    "${POPTRACKER_ROOT}/src/uilib/*.cpp"
    "${POPTRACKER_ROOT}/src/ui/*.cpp"
    "${POPTRACKER_ROOT}/src/core/*.cpp"
    "${POPTRACKER_ROOT}/src/luasandbox/*.cpp"
    "${POPTRACKER_ROOT}/src/usb2snes/*.cpp"
    "${POPTRACKER_ROOT}/src/uat/*.cpp"
    "${POPTRACKER_ROOT}/src/ap/*.cpp"
    "${POPTRACKER_ROOT}/src/luaconnector/*.cpp"
    "${POPTRACKER_ROOT}/src/http/*.cpp"
    "${POPTRACKER_ROOT}/src/packmanager/*.cpp"
)

file(GLOB LUA_SOURCES CONFIGURE_DEPENDS "${POPTRACKER_ROOT}/lib/lua/l*.c")
list(REMOVE_ITEM LUA_SOURCES
    "${POPTRACKER_ROOT}/lib/lua/lua.c"
    "${POPTRACKER_ROOT}/lib/lua/luac.c"
)
set_source_files_properties(${LUA_SOURCES} PROPERTIES LANGUAGE CXX)

# Lua enables 64-bit stdio offsets by default. On Android's 32-bit ABIs the
# corresponding fseeko/ftello symbols require API 24, but the app supports API
# 21. Tracker scripts do not need multi-gigabyte files, so retain the API 21
# compatible stdio API on those ABIs.
if(ANDROID_ABI MATCHES "^(armeabi-v7a|x86)$")
    set_source_files_properties(${LUA_SOURCES} PROPERTIES
        COMPILE_DEFINITIONS "_FILE_OFFSET_BITS=32"
    )
endif()

add_library(main SHARED
    ${POPTRACKER_SOURCES}
    ${LUA_SOURCES}
    "${POPTRACKER_ROOT}/lib/fmt/src/format.cc"
    "${CMAKE_CURRENT_LIST_DIR}/android_bridge.cpp"
)

# Keep the upstream wswrap submodule pinned to a public commit while applying
# the Android resume fix in this repository. Android may suspend polling long
# enough for a WebSocket ping to hit a stale connection; upstream's throwing
# close overload then terminates the process. Generate an include overlay that
# uses the exception-free overload so fresh recursive clones retain the fix.
set(WSWRAP_ANDROID_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/wswrap-android-include")
file(MAKE_DIRECTORY "${WSWRAP_ANDROID_INCLUDE_DIR}")
configure_file(
    "${POPTRACKER_ROOT}/lib/wswrap/include/wswrap.hpp"
    "${WSWRAP_ANDROID_INCLUDE_DIR}/wswrap.hpp"
    COPYONLY
)
file(READ
    "${POPTRACKER_ROOT}/lib/wswrap/include/wswrap_websocketpp.hpp"
    WSWRAP_WEBSOCKETPP_HEADER
)
set(WSWRAP_PING_CLOSE_ORIGINAL [=[
                    if (ping_ec) {
                        if (_herror) _herror(ec.message());
                        impl->second->close(websocketpp::close::status::internal_endpoint_error, "send failed");
                    }
]=])
set(WSWRAP_PING_CLOSE_ANDROID [=[
                    if (ping_ec) {
                        if (_herror) _herror(ping_ec.message());
                        websocketpp::lib::error_code close_ec;
                        impl->second->close(
                            websocketpp::close::status::internal_endpoint_error,
                            "send failed",
                            close_ec
                        );
                        if (close_ec && _herror)
                            _herror(close_ec.message());
                        return;
                    }
]=])
string(FIND
    "${WSWRAP_WEBSOCKETPP_HEADER}"
    "${WSWRAP_PING_CLOSE_ORIGINAL}"
    WSWRAP_PING_CLOSE_POSITION
)
if(WSWRAP_PING_CLOSE_POSITION EQUAL -1)
    message(FATAL_ERROR "Unable to apply Android wswrap resume fix")
endif()
string(REPLACE
    "${WSWRAP_PING_CLOSE_ORIGINAL}"
    "${WSWRAP_PING_CLOSE_ANDROID}"
    WSWRAP_WEBSOCKETPP_HEADER
    "${WSWRAP_WEBSOCKETPP_HEADER}"
)
file(WRITE
    "${WSWRAP_ANDROID_INCLUDE_DIR}/wswrap_websocketpp.hpp"
    "${WSWRAP_WEBSOCKETPP_HEADER}"
)

target_include_directories(main PRIVATE
    "${POPTRACKER_ROOT}/src"
    "${POPTRACKER_ROOT}/lib"
    "${POPTRACKER_ROOT}/lib/lua"
    "${POPTRACKER_ROOT}/lib/asio/include"
    "${POPTRACKER_ROOT}/lib/miniz"
    "${POPTRACKER_ROOT}/lib/json/include"
    "${POPTRACKER_ROOT}/lib/valijson/include"
    "${WSWRAP_ANDROID_INCLUDE_DIR}"
    "${POPTRACKER_ROOT}/lib/wswrap/include"
    "${POPTRACKER_ROOT}/lib/fmt/include"
    "${CMAKE_CURRENT_LIST_DIR}"
    "${SDL2_COMPAT_INCLUDE_DIR}"
)

target_compile_definitions(main PRIVATE
    ANDROID
    ASIO_STANDALONE
    JSON_HAS_CPP_17
    LUA_CPP
    LUA_USE_POSIX
    WITHOUT_UPDATE_CHECK
    _REENTRANT
)

target_compile_options(main PRIVATE
    -Wall
    -Wextra
    -Wno-deprecated-declarations
    -Wno-unused-function
)

target_link_options(main PRIVATE "-Wl,-z,max-page-size=16384")

find_library(ANDROID_LIBRARY android)
find_library(DL_LIBRARY dl)
find_library(LOG_LIBRARY log)
find_library(Z_LIBRARY z)

target_link_libraries(main PRIVATE
    SDL2::SDL2
    SDL2_image::SDL2_image
    SDL2_ttf::SDL2_ttf
    openssl::ssl
    openssl::crypto
    ${ANDROID_LIBRARY}
    ${DL_LIBRARY}
    ${LOG_LIBRARY}
    ${Z_LIBRARY}
)
