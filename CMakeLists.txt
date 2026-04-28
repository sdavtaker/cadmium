cmake_minimum_required(VERSION 3.28)

project(cadmium VERSION 0.2.9 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "")
endif()

# ----------------------------------------------------------------------------
# cadmium INTERFACE library
# ----------------------------------------------------------------------------

add_library(cadmium INTERFACE)
add_library(cadmium::cadmium ALIAS cadmium)

target_include_directories(cadmium INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_compile_features(cadmium INTERFACE cxx_std_23)
target_compile_options(cadmium INTERFACE -Wall -Wextra -pedantic)

# ----------------------------------------------------------------------------
# Examples (pdevs only)
# ----------------------------------------------------------------------------

option(CADMIUM_BUILD_EXAMPLES "Build examples" ON)
if(CADMIUM_BUILD_EXAMPLES)
    add_executable(main-clock  example/pdevs/main-clock.cpp)
    add_executable(main-count-fives example/pdevs/main-count-fives.cpp)
    target_link_libraries(main-clock        PRIVATE cadmium)
    target_link_libraries(main-count-fives  PRIVATE cadmium)
endif()

# ----------------------------------------------------------------------------
# Tests (wired per batch in later migration steps)
# ----------------------------------------------------------------------------

option(CADMIUM_BUILD_TESTS "Build tests" OFF)
if(CADMIUM_BUILD_TESTS)
    find_package(Catch2 CONFIG REQUIRED)
    enable_testing()

    # Batch 1: pdevs static engine (memory-vault-9py)
    foreach(test_name
            pdevs_simulator_test
            pdevs_coordinator_test
            pdevs_runner_test)
        add_executable(${test_name} test/${test_name}.cpp)
        target_link_libraries(${test_name} PRIVATE cadmium Catch2::Catch2WithMain)
        add_test(NAME ${test_name} COMMAND ${test_name})
    endforeach()

    # Batch 2: basic models + dynamic engine (memory-vault-jlw)
    foreach(test_name
            devs_accumulator_model_test
            devs_generator_model_test
            devs_passive_model_test
            pdevs_accumulator_model_test
            pdevs_generator_model_test
            pdevs_passive_model_test
            tuple_to_ostream_test
            logger_test
            pdevs_engine_helper_test
            pdevs_dynamic_atomic_test
            pdevs_dynamic_atomic_helpers_test
            pdevs_dynamic_engine_helpers_test
            pdevs_dynamic_model_helpers_test
            pdevs_dynamic_simulator_test
            pdevs_dynamic_coordinator_test
            pdevs_dynamic_runner_test
            pdevs_boxes_cleanup_test)
        add_executable(${test_name} test/${test_name}.cpp)
        target_link_libraries(${test_name} PRIVATE cadmium Catch2::Catch2WithMain)
        add_test(NAME ${test_name} COMMAND ${test_name})
    endforeach()
endif()

# ----------------------------------------------------------------------------
# Compile-check tests (memory-vault-y61)
# compile-fails/ skipped until C++23 concepts land (memory-vault-quk)
# ----------------------------------------------------------------------------

option(CADMIUM_BUILD_COMPILE_TESTS "Build compile-check tests" OFF)
if(CADMIUM_BUILD_COMPILE_TESTS)
    if(NOT CADMIUM_BUILD_TESTS)
        enable_testing()
    endif()
    foreach(compile_test
            devs_generator_atomic_check_test
            devs_passive_atomic_check_test
            pdevs_coordinator_of_coupled_of_atomics_test
            pdevs_coordinator_of_coupled_of_mixed_models_test
            pdevs_coordinator_of_empty_coupled_model_test
            pdevs_coupled_of_all_atomics_test
            pdevs_coupled_of_all_coupleds_test
            pdevs_coupled_of_mixed_models_test
            pdevs_empty_coupled_model_is_a_valid_model_test
            pdevs_generator_atomic_check_test
            pdevs_passive_atomic_check_test
            pdevs_simulator_provide_types_test
            pdevs_valid_atomic_check_test
            pdevs_valid_atomic_with_multiple_ports_check_test)
        add_executable(${compile_test} EXCLUDE_FROM_ALL
            test-compile/compiles/${compile_test}.cpp)
        target_link_libraries(${compile_test} PRIVATE cadmium)
        add_test(
            NAME compile_check_${compile_test}
            COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target ${compile_test})
        set_tests_properties(compile_check_${compile_test}
            PROPERTIES WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
    endforeach()
endif()

# ----------------------------------------------------------------------------
# Install
# ----------------------------------------------------------------------------

include(GNUInstallDirs)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/cadmium/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/cadmium
)

include(CMakePackageConfigHelpers)

install(TARGETS cadmium EXPORT cadmium)

install(EXPORT cadmium
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/cadmium"
    NAMESPACE cadmium::
    FILE cadmiumConfig.cmake
)

write_basic_package_version_file(
    ${CMAKE_BINARY_DIR}/cadmiumConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

install(
    FILES ${CMAKE_BINARY_DIR}/cadmiumConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/cadmium
)

# ----------------------------------------------------------------------------
# clang-format targets
# ----------------------------------------------------------------------------

find_program(CLANG_FORMAT clang-format)
if(CLANG_FORMAT)
    file(GLOB_RECURSE ALL_SOURCE_FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/test/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/example/pdevs/*.cpp"
    )
    add_custom_target(format
        COMMAND ${CLANG_FORMAT} -i ${ALL_SOURCE_FILES}
        COMMENT "Applying clang-format"
    )
    add_custom_target(check-format
        COMMAND ${CLANG_FORMAT} --dry-run --Werror ${ALL_SOURCE_FILES}
        COMMENT "Checking clang-format"
    )
endif()
