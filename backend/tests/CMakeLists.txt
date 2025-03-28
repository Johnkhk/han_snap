cmake_minimum_required(VERSION 3.10)
project(HansnapTests)

include(FetchContent)

# Update to a working GoogleTest URL
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(googletest)

# Tests link against the database library
add_executable(database_tests database_tests.cpp)
target_link_libraries(database_tests
  database_lib  # This gives you the MySQL dependencies transitively
  gtest_main
)

include(GoogleTest)
gtest_discover_tests(database_tests)