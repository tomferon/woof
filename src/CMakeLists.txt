add_library(woof_core)

target_sources(woof_core
  PUBLIC
    FILE_SET woof_modules TYPE CXX_MODULES
    FILES
      engine.cppm
      jobs.cppm

  PRIVATE
    engine.cpp
    jobs.cpp
)

add_executable(woof)

target_link_libraries(woof PRIVATE woof_core)

target_sources(woof
    PRIVATE
        main.cpp
)
