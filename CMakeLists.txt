cmake_minimum_required(VERSION 3.10)
project(han_snap VERSION 1.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find wxWidgets
find_package(wxWidgets REQUIRED COMPONENTS core base)
include(${wxWidgets_USE_FILE})

# Add include directory
include_directories(${CMAKE_SOURCE_DIR}/include)

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

# Add the source files
add_executable(han_snap_app 
    src/main.cpp
    src/main_frame.cpp
    src/screenshot.cpp
    src/hotkey.cpp
    src/filemonitor.cpp
    src/clipboard_processor.cpp
    src/taskbar.cpp
)

target_link_libraries(han_snap_app PRIVATE ${wxWidgets_LIBRARIES} ${EXTRA_LIBS})

# Add the examples
add_executable(hello_world examples/hello_world.cpp)
target_link_libraries(hello_world PRIVATE ${wxWidgets_LIBRARIES})
