cmake_minimum_required(VERSION 3.10)
project(han_snap VERSION 1.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find wxWidgets
find_package(wxWidgets REQUIRED COMPONENTS core base)
include(${wxWidgets_USE_FILE})

# Direct Homebrew path
set(HOMEBREW_PATH "/opt/homebrew")

# Add include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${HOMEBREW_PATH}/include
)

# Print all files in the lib directory to debug
message(STATUS "Listing files in ${HOMEBREW_PATH}/lib:")
file(GLOB LIB_FILES "${HOMEBREW_PATH}/lib/*")
foreach(F IN LISTS LIB_FILES)
    message(STATUS "Found library file: ${F}")
endforeach()

# Find Tesseract library
find_library(TESSERACT_LIBRARY
    NAMES tesseract
    PATHS ${HOMEBREW_PATH}/lib
    REQUIRED)

# Find Leptonica library - try different possible names and patterns
find_library(LEPTONICA_LIBRARY
    NAMES lept liblept.dylib liblept.a liblept leptonica
    PATHS 
        ${HOMEBREW_PATH}/lib
        ${HOMEBREW_PATH}/Cellar/leptonica/*/lib
    NO_DEFAULT_PATH)

if(NOT LEPTONICA_LIBRARY)
    message(FATAL_ERROR "Leptonica library not found. Please install it with 'brew install leptonica'")
endif()

message(STATUS "Found Tesseract lib: ${TESSERACT_LIBRARY}")
message(STATUS "Found Leptonica lib: ${LEPTONICA_LIBRARY}")

# Platform-specific settings
if(APPLE)
    find_library(CARBON_LIBRARY Carbon)
    list(APPEND EXTRA_LIBS ${CARBON_LIBRARY})
elseif(UNIX AND NOT APPLE)
    find_package(X11 REQUIRED)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
    list(APPEND EXTRA_LIBS ${X11_LIBRARIES} ${GTK3_LIBRARIES})
    include_directories(${X11_INCLUDE_DIR} ${GTK3_INCLUDE_DIRS})
endif()

# Find libcurl
find_package(CURL REQUIRED)
include_directories(${CURL_INCLUDE_DIRS})

# Find nlohmann_json
find_package(nlohmann_json REQUIRED)

# Add the source files
add_executable(han_snap_app 
    src/main.cpp
    src/main_frame.cpp
    src/screenshot.cpp
    src/hotkey.cpp
    src/filemonitor.cpp
    src/clipboard_processor.cpp
    src/taskbar.cpp
    src/ocr.cpp
    src/http_client.cpp
)

target_link_libraries(han_snap_app PRIVATE 
    ${wxWidgets_LIBRARIES} 
    ${EXTRA_LIBS}
    ${TESSERACT_LIBRARY}
    ${LEPTONICA_LIBRARY}
    ${CURL_LIBRARIES}
    nlohmann_json::nlohmann_json
)

# Add the examples
add_executable(hello_world examples/hello_world.cpp)
target_link_libraries(hello_world PRIVATE ${wxWidgets_LIBRARIES})
