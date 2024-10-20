cmake_minimum_required(VERSION 3.27)

include(cmake/modules.cmake)
include(GitVersion)

project(http
    VERSION ${GIT_VERSION_ABBREV}
    DESCRIPTION "Zipline serialization and protocol utilities for C++"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS NO)

include(ProjectTesting)
include(packages.cmake)

add_library(http)
add_library(http::http ALIAS http)
target_sources(http PUBLIC FILE_SET HEADERS BASE_DIRS include)
target_include_directories(http
    PUBLIC
        ${CURL_INCLUDE_DIRS}
        ${NGHTTP2_INCLUDE_DIRS}
)
target_link_libraries(http
    PUBLIC
        ${CURL_LIBRARIES}
        ext::ext
        fmt::fmt
        netcore::netcore
        ${NGHTTP2_LIBRARIES}
        nlohmann_json::nlohmann_json
        timber::timber
        uuidcpp::uuid++
)

if(PROJECT_TESTING)
    find_package(Python REQUIRED)

    add_executable(http.test)
    target_link_libraries(http.test PRIVATE
        GTest::gtest
        http::http
    )

    set(test_dir "${PROJECT_SOURCE_DIR}/test")
    add_custom_command(
        OUTPUT ${test_dir}/test
        COMMAND go
        ARGS build
        WORKING_DIRECTORY ${test_dir}
    )
    add_custom_target(test_server DEPENDS ${test_dir}/test)
    add_dependencies(http.test test_server)

    add_test(
        NAME "Unit Tests"
        COMMAND ${Python_EXECUTABLE} ${test_dir}/run.py $<TARGET_FILE:http.test>
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    )
endif()

add_subdirectory(include)
add_subdirectory(src)

include(Sanitizers)
enable_asan()

include(EasyInstall)
easy_install(
    TARGETS http
    DEPENDENCIES
        ext
        fmt
        netcore
        timber
        uuidcpp
)

if(PROJECT_IS_TOP_LEVEL)
    add_custom_target(docker
        docker build --progress=plain --no-cache .
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
        USES_TERMINAL
    )
endif()
