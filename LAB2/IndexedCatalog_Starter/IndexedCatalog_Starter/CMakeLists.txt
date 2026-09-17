cmake_minimum_required(VERSION 3.20)
project(Lab2IndexedCatalog LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(catalog_core
    src/binary_io.cpp
    src/crc32.cpp
    src/catalog_codec.cpp
    src/index_io.cpp
    src/catalog.cpp
)

target_include_directories(catalog_core PUBLIC include)

if(MSVC)
    target_compile_options(catalog_core PRIVATE /W4)
else()
    target_compile_options(catalog_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(lab2_catalog src/main.cpp)
target_link_libraries(lab2_catalog PRIVATE catalog_core)

add_executable(catalog_generate tools/generate_catalog.cpp)
target_link_libraries(catalog_generate PRIVATE catalog_core)

add_executable(catalog_corrupt tools/corrupt_file.cpp)

add_executable(catalog_tests
    tests/tests.cpp
    tests/student_tests.cpp
)
target_link_libraries(catalog_tests PRIVATE catalog_core)
target_include_directories(catalog_tests PRIVATE third_party)

enable_testing()
add_test(NAME catalog_tests COMMAND catalog_tests)
