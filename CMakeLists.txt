
# CMake project file.
# Set everything up before commencing with any builds.

# At the time, Gavin was using version 3.7.
cmake_minimum_required(VERSION 3.9)

find_program(CCACHE_PROGRAM ccache)
if (CCACHE_PROGRAM)

    message("NOTE: Using CCache for caching.")

    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
endif ()

project(SwsExample)

message(STATUS "Project:    ${PROJECT_NAME}")
message(STATUS "Platform:   ${CMAKE_SYSTEM_NAME} (${CMAKE_SYSTEM})")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)


include(ExternalProject)

################################################################
# Find Boost.
# This flag may need to change depending on the version of
# Boost installed.
set(Boost_USE_MULTITHREADED ON)

find_package(Boost REQUIRED COMPONENTS 
    system
    )

################################################################
# Git (to pull down packages)
find_package(Git REQUIRED)

set(EXTERNAL_INSTALL_LOCATION ${CMAKE_BINARY_DIR}/external)

################################################################
# Eidheim's Simple Web Server.
ExternalProject_Add(
    eh_swserver
    GIT_REPOSITORY https://gitlab.com/eidheim/Simple-Web-Server.git
    GIT_SHALLOW 1 # (ambiguous when using GIT_TAG)
    #GIT_PROGRESS 1
    #GIT_TAG b8de3ae50484eb9e21eb94c67c754ecc27b271a5 # Release 3.0.0 (latest) 9th Jun
    TIMEOUT 60
    CMAKE_ARGS -DCMAKE_BUILD_TYPE=RELWITHDEBINFO -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DBUILD_TESTING=OFF -DCMAKE_INSTALL_PREFIX=${EXTERNAL_INSTALL_LOCATION}
)

# Set the location where our binaries will be stored.
# WARN/TODO: Not quite right, because .lib or .a files should
#            not go in the bin folder!
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include_directories(${EXTERNAL_INSTALL_LOCATION}/include)
link_directories(${EXTERNAL_INSTALL_LOCATION}/lib)
link_directories(${CMAKE_BINARY_DIR}/bin)

# Local header files here ONLY
set(TARGET_HPP
    )

# Local source files here
set(TARGET_CPP
    main.cpp
    )

# Define an executable
add_executable(sws_example ${TARGET_HPP} ${TARGET_CPP})

# External dependencies
add_dependencies(sws_example
	eh_swserver
	)

target_link_libraries(sws_example

	Boost::system
	
	pthread
	dl
    )
