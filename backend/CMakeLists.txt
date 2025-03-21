cmake_minimum_required(VERSION 3.10)
project(drogon_backend)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Drogon
find_package(Drogon REQUIRED)

# Find libcurl
find_package(CURL REQUIRED)
include_directories(${CURL_INCLUDE_DIRS})

# Find nlohmann_json
find_package(nlohmann_json REQUIRED)

# Find Boost with more detailed configuration
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
# For macOS with Homebrew, specify the Boost root directory
if(APPLE)
    set(BOOST_ROOT /opt/homebrew/Cellar/boost/1.87.0_1)
    set(Boost_NO_SYSTEM_PATHS ON)
endif()
find_package(Boost COMPONENTS system REQUIRED)
if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
    message(STATUS "Boost found: ${Boost_INCLUDE_DIRS}")
else()
    message(FATAL_ERROR "Could not find Boost - please install Boost libraries")
endif()

# Include directories - include both backend and main project includes
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include  # This points to the main project's include directory
)

# Add the source files
add_executable(drogon_backend 
    src/main.cpp
    src/llm.cpp
)

# Link libraries
target_link_libraries(drogon_backend PRIVATE 
    Drogon::Drogon
    ${CURL_LIBRARIES}
    nlohmann_json::nlohmann_json
    ${Boost_LIBRARIES}
)
