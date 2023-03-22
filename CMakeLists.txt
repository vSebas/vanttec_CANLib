cmake_minimum_required(VERSION 3.5)
project(CANLib)

set(CMAKE_CXX_STANDARD 14)

file(GLOB_RECURSE CANLib_SRC src/*.cpp)

add_library(CANLibLIB ${CANLib_SRC})

include_directories(src src/CANLib)
add_executable(CANLib main.cpp)
target_link_libraries(CANLib pthread CANLibLIB)


file(GLOB_RECURSE CANLib_Tests tests/*.cpp)

if(${CMAKE_VERSION} VERSION_GREATER 3.11)
include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/609281088cfefc76f9d0ce82e1ff6c30cc3591e5.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()
add_executable(CANLib_test ${CANLib_Tests})
target_link_libraries(CANLib_test gtest_main CANLibLIB)

include(GoogleTest)
gtest_discover_tests(CANLib_test)
endif()