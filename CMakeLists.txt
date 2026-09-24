cmake_minimum_required(VERSION 3.23)

project(pibase
    VERSION 0.1.0
    DESCRIPTION "The PI family base layer: dependency-free C vocabulary shared by every PI library"
    HOMEPAGE_URL "https://gitee.com/Aeroscis/pibase"
    LANGUAGES C CXX)

option(PI_BASE_BUILD_TESTS "Build pibase's own self-test" ON)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# ---------------------------------------------------------------------------
# The library itself: INTERFACE, because there is nothing to compile.
#
# The root layer deliberately produces no binary. No binary means no ABI to
# freeze, and that is precisely the property the one layer that must never
# break wants: the contract is the header, and a consumer recompiles to
# adopt it. Keeping it an INTERFACE target also means find_package(pibase)
# gives consumers headers and compile features and nothing to link against.
# ---------------------------------------------------------------------------
add_library(pibase INTERFACE)
add_library(pibase::pibase ALIAS pibase)

target_include_directories(pibase INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

# `static inline` and <stdint.h> - C99 is the floor for consumers. C++ consumers
# inherit nothing here: the header is valid C++, so no C++ feature is required.
target_compile_features(pibase INTERFACE c_std_99)

# ---------------------------------------------------------------------------
# Install and package config
# ---------------------------------------------------------------------------
install(TARGETS pibase EXPORT pibaseTargets)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

set(PIBASE_CMAKE_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/pibase")

install(EXPORT pibaseTargets
    FILE pibaseTargets.cmake
    NAMESPACE pibase::
    DESTINATION "${PIBASE_CMAKE_INSTALL_DIR}")

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/pibaseConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/pibaseConfig.cmake"
    INSTALL_DESTINATION "${PIBASE_CMAKE_INSTALL_DIR}")

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/pibaseConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/pibaseConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/pibaseConfigVersion.cmake"
    DESTINATION "${PIBASE_CMAKE_INSTALL_DIR}")

# ---------------------------------------------------------------------------
# Self-test
# ---------------------------------------------------------------------------
if(PI_BASE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
