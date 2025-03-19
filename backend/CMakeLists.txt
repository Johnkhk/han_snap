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
)
