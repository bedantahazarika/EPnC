cmake_minimum_required(VERSION 3.10...3.30)

project(EPnC C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
add_compile_definitions(_CRT_SECURE_NO_WARNINGS)

#Without a build type, Makefiles and Ninja build WITHOUT optimization, and the neural fitting is about 10 times slower.
#(In Visual Studio, CLion or Xcode pick the Release configuration yourself.)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()



#=====================================================================================================
# SWITCH: build the app (needs Raylib)?   Change ON to OFF on the next line to build only the tests.
# From a terminal you can also decide per build, without editing this file:   cmake -S . -B build -DWITH_GUI=OFF
#=====================================================================================================
if(NOT DEFINED WITH_GUI)
    set(WITH_GUI ON)
endif()

if(WITH_GUI)
    add_compile_definitions(WITH_GUI)
    #Use an installed raylib if there is one, otherwise download version 5.5 once and build it here (needs git and internet).
    find_package(raylib 5.0 QUIET)
    if(NOT raylib_FOUND)
        include(FetchContent)
        set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
        FetchContent_Declare(raylib GIT_REPOSITORY https://github.com/raysan5/raylib.git GIT_TAG 5.5 GIT_SHALLOW TRUE)
        FetchContent_MakeAvailable(raylib)
    endif()
endif()



#=====================================================================================================
# Tests: every file  tests/test_*.c  becomes its own program, named like the file. Nothing to edit when you add a test
# (just run CMake again so it notices the new file).
#=====================================================================================================
file(GLOB TEST_SOURCES CONFIGURE_DEPENDS tests/test_*.c)
foreach(TestSource ${TEST_SOURCES})
    get_filename_component(TestName ${TestSource} NAME_WE)
    add_executable(${TestName} ${TestSource})
    target_include_directories(${TestName} PRIVATE Core/)
    if(NOT WIN32)
        target_link_libraries(${TestName} m)
    endif()
endforeach()



#=====================================================================================================
# The app. The quotes around the file name matter: an unquoted # would start a CMake comment.
#=====================================================================================================
if(WITH_GUI)
    add_executable(EPnC_App "Core/#MainApp.c")
    target_include_directories(EPnC_App PRIVATE Core/)
    target_link_libraries(EPnC_App raylib)
    if(NOT WIN32)
        target_link_libraries(EPnC_App m)
    endif()
endif()
