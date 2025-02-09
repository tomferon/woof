add_library(woof_core)

target_sources(woof_core
    PUBLIC FILE_SET woof_modules TYPE CXX_MODULES FILES
    engine.cppm
    jobs.cppm
    server.cppm
    http_server.cppm
    api.cppm
    cli_options.cppm

    PRIVATE
    engine.cpp
    jobs.cpp
    # server.cpp
    http_server.cpp
    api.cpp
    cli_options.cpp
)

target_link_libraries(woof_core PRIVATE nlohmann_json::nlohmann_json Boost::program_options)

target_include_directories(woof_core PRIVATE ${Boost_INCLUDE_DIRS})

add_executable(woof)

target_link_libraries(woof PRIVATE woof_core)

target_sources(woof PRIVATE main.cpp)
