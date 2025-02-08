cmake_minimum_required(VERSION 3.30 FATAL_ERROR)

project(woof VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CXX_STANDARD_REQUIRED YES)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(CTest)

# Fix problems with clangd not finding standard libraries.
# See https://discourse.nixos.org/t/get-clangd-to-find-standard-headers-in-nix-shell/11268/10
if(CMAKE_EXPORT_COMPILE_COMMANDS)
    set(CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES
        ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES})
endif()

find_package(Catch2 3.7.1 REQUIRED)
find_package(nlohmann_json 3.11.3 REQUIRED)

add_subdirectory(src)
add_subdirectory(test)