cmake_minimum_required(VERSION 3.10)
project(drogon_backend)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Use standard CMake modules first
set(CMAKE_MODULE_PATH ${CMAKE_ROOT}/Modules ${CMAKE_MODULE_PATH})

# Find MySQL before Drogon
# set(MySQL_FOUND TRUE)
# find_package(MySQL REQUIRED)
# message(STATUS "Found MySQL: ${MYSQL_LIBRARY} (found version ${MYSQL_VERSION})")
# message(STATUS "MySQL include dir: ${MYSQL_INCLUDE_DIR}")

# Find MySQL Connector/C++ with correct name
find_package(mysql-concpp REQUIRED)
message(STATUS "Found MySQL Connector/C++")

# Create our own MySQL interface that Drogon expects
add_library(MySQL_lib INTERFACE IMPORTED)
target_include_directories(MySQL_lib INTERFACE ${MYSQL_INCLUDE_DIR})
target_link_libraries(MySQL_lib INTERFACE ${MYSQL_LIBRARY})

# Tell Drogon MySQL is already found
set(MySQL_FOUND TRUE)

# Now find Drogon
find_package(Drogon CONFIG REQUIRED)

# Find libcurl
find_package(CURL REQUIRED)

# Find nlohmann_json
find_package(nlohmann_json REQUIRED)

# Boost configuration
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)

# Optional: allow user to set BOOST_ROOT externally, but don't hardcode mac paths
find_package(Boost COMPONENTS system REQUIRED)

if(Boost_FOUND)
    message(STATUS "Boost found: ${Boost_INCLUDE_DIRS}")
else()
    message(FATAL_ERROR "Could not find Boost - please install Boost libraries")
endif()

# OPTIONAL: Only try to find mysql-concpp if you really need it
# If you don't need it, comment this out
# find_package(mysql-concpp CONFIG QUIET)

# Create a library for the database code
add_library(database_lib STATIC
    src/database.cpp
)

# Include paths for database_lib
target_include_directories(database_lib PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${MYSQL_INCLUDE_DIR}
)

# Link MySQL libraries
target_link_libraries(database_lib PUBLIC
    ${MYSQL_LIBRARY}
    mysql::concpp  # This is the correct target name for mysql-concpp
)

# Find OpenSSL
find_package(OpenSSL REQUIRED)
message(STATUS "Found OpenSSL: ${OPENSSL_LIBRARIES}")

# Create main executable
add_executable(drogon_backend 
    src/main.cpp
    src/llm.cpp
)

# Include headers
target_include_directories(drogon_backend PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Link everything together
target_link_libraries(drogon_backend PRIVATE 
    database_lib
    Drogon::Drogon
    ${CURL_LIBRARIES}
    nlohmann_json::nlohmann_json
    ${Boost_LIBRARIES}
    OpenSSL::SSL
    OpenSSL::Crypto
)

# Enable testing for the backend
enable_testing()
add_subdirectory(tests)
