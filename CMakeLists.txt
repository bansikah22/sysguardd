cmake_minimum_required(VERSION 3.20)
project(sysguardd LANGUAGES C CXX)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(SYSGUARDD_ENABLE_SANITIZERS "Enable ASAN/UBSAN in Debug" ON)

if(CMAKE_BUILD_TYPE STREQUAL "Debug" AND SYSGUARDD_ENABLE_SANITIZERS)
  add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
  add_link_options(-fsanitize=address,undefined)
endif()

add_library(sysguardd_core
  src/config.cpp
  src/cli.cpp
  src/policy_engine.cpp
  src/mitigator.cpp
  src/audit_logger.cpp
  src/alert_dispatcher.cpp
  src/service.cpp
)

target_include_directories(sysguardd_core PUBLIC include)
target_compile_options(sysguardd_core PRIVATE -Wall -Wextra -Wpedantic -Werror)

target_link_libraries(sysguardd_core PRIVATE pthread)

add_executable(sysguardd src/main.cpp)
target_link_libraries(sysguardd PRIVATE sysguardd_core)

enable_testing()
add_executable(sysguardd_policy_test tests/test_policy.cpp)
target_link_libraries(sysguardd_policy_test PRIVATE sysguardd_core)
add_test(NAME sysguardd_policy_test COMMAND sysguardd_policy_test)
