add_executable(woof_tests)

target_link_libraries(woof_tests
  PRIVATE
    woof_core
    Catch2::Catch2WithMain
)

target_sources(woof_tests
  PUBLIC
    FILE_SET woof_test_modules TYPE CXX_MODULES
    FILES
      driver.cppm

  PRIVATE
    tests.cpp # FIXME
    jobs_test.cpp
    driver.cpp
)

include(Catch)
catch_discover_tests(woof_tests)
