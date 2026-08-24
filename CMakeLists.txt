cmake_minimum_required(VERSION 3.16)
project(sentry_bee C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_library(sentry_bee_core
    firmware/src/features.c
    firmware/src/app.c
    firmware/src/telemetry.c
    firmware/src/drivers.c
)

target_include_directories(sentry_bee_core PUBLIC firmware/include)
target_link_libraries(sentry_bee_core PUBLIC m)

target_compile_options(sentry_bee_core PRIVATE -Wall -Wextra -Wpedantic)

add_executable(sentry_bee_host firmware/src/main_host.c)
target_link_libraries(sentry_bee_host PRIVATE sentry_bee_core)

enable_testing()
add_executable(test_features firmware/tests/test_features.c)
target_link_libraries(test_features PRIVATE sentry_bee_core)
add_test(NAME features COMMAND test_features)
