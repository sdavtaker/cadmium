cmake_minimum_required(VERSION 3.28)

# Toolchain must be set before project() so CMake loads vcpkg during compiler detection.
if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "")
endif()

project(cadmium VERSION 0.3.1 LANGUAGES CXX)

set(CMAKE_CXX_EXTENSIONS OFF)

include(GNUInstallDirs)

# ----------------------------------------------------------------------------
# cadmium INTERFACE library
# ----------------------------------------------------------------------------

find_package(spdlog CONFIG REQUIRED)

add_library(cadmium INTERFACE)
add_library(cadmium::cadmium ALIAS cadmium)

target_include_directories(cadmium INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_compile_features(cadmium INTERFACE cxx_std_23)
# Warnings apply only when building from source tree, not when installed via vcpkg.
target_compile_options(cadmium INTERFACE
    $<BUILD_INTERFACE:-Wall -Wextra -pedantic>
)
target_link_libraries(cadmium INTERFACE spdlog::spdlog)

# ----------------------------------------------------------------------------
# Examples
# ----------------------------------------------------------------------------

option(CADMIUM_BUILD_EXAMPLES "Build examples" ${PROJECT_IS_TOP_LEVEL})
if(CADMIUM_BUILD_EXAMPLES)
    add_executable(main-clock                       example/pdevs/main-clock.cpp)
    add_executable(main-count-fives                 example/pdevs/main-count-fives.cpp)
    add_executable(devs-main-clock                  example/devs/main-clock.cpp)
    add_executable(devs-main-count-fives            example/devs/main-count-fives.cpp)
    add_executable(stdevs-main-bernoulli-generator  example/stdevs/main-bernoulli-generator.cpp)
    add_executable(stdevs-main-stochastic-channel   example/stdevs/main-stochastic-channel.cpp)
    add_executable(qss-main-lotka-volterra           example/qss/main-lotka-volterra.cpp)
    foreach(tgt
            main-clock main-count-fives
            devs-main-clock devs-main-count-fives
            stdevs-main-bernoulli-generator stdevs-main-stochastic-channel
            qss-main-lotka-volterra)
        target_link_libraries(${tgt} PRIVATE cadmium)
    endforeach()
endif()

# ----------------------------------------------------------------------------
# Tests (wired per batch in later migration steps)
# ----------------------------------------------------------------------------

option(CADMIUM_BUILD_TESTS "Build tests" ${PROJECT_IS_TOP_LEVEL})
if(CADMIUM_BUILD_TESTS)
    find_package(Catch2 CONFIG REQUIRED)
    enable_testing()

    # Batch 0: concept-satisfaction tests
    foreach(test_name
            pdevs_concepts_test
            stdevs_concepts_test)
        add_executable(${test_name} test/${test_name}.cpp)
        target_link_libraries(${test_name} PRIVATE cadmium Catch2::Catch2WithMain)
        add_test(NAME ${test_name} COMMAND ${test_name})
    endforeach()

    # Batch 1: pdevs static engine (memory-vault-9py)
    foreach(test_name
            pdevs_simulator_test
            pdevs_coordinator_test
            pdevs_runner_test)
        add_executable(${test_name} test/${test_name}.cpp)
        target_link_libraries(${test_name} PRIVATE cadmium Catch2::Catch2WithMain)
        add_test(NAME ${test_name} COMMAND ${test_name})
    endforeach()

    # Batch 3: devs static engine
    foreach(test_name
            devs_simulator_test
            devs_coordinator_test
            devs_runner_test)
        add_executable(${test_name} test/${test_name}.cpp)
        target_link_libraries(${test_name} PRIVATE cadmium Catch2::Catch2WithMain)
        add_test(NAME ${test_name} COMMAND ${test_name})
    endforeach()

    # Batch 4: stdevs static engine
    foreach(test_name
            stdevs_simulator_test
            stdevs_coordinator_test
            stdevs_runner_test)
        add_executable(${test_name} test/${test_name}.cpp)
        target_link_libraries(${test_name} PRIVATE cadmium Catch2::Catch2WithMain)
        add_test(NAME ${test_name} COMMAND ${test_name})
    endforeach()

    # Batch 5: QSS1 atomics and experiments
    foreach(test_name
            qss1_integrator_test
            qss1_lotka_volterra_test)
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
# Compile-check tests replaced by pdevs_concepts_test (memory-vault-dwr).
# The old test-compile/ directory has been deleted.
# ----------------------------------------------------------------------------

# ----------------------------------------------------------------------------
# Install
# ----------------------------------------------------------------------------


install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/cadmium/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/cadmium
)

install(
    TARGETS cadmium
    EXPORT cadmiumTargets
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(
    EXPORT cadmiumTargets
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/cadmium"
    NAMESPACE cadmium::
    FILE cadmiumTargets.cmake
)

include(CMakePackageConfigHelpers)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cadmiumConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/cadmiumConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/cadmium
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/cadmiumConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMinorVersion
)

install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/cadmiumConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/cadmiumConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/cadmium
)

# ----------------------------------------------------------------------------
# clang-format targets
# ----------------------------------------------------------------------------

find_program(CLANG_FORMAT clang-format)
if(CLANG_FORMAT)
    file(GLOB_RECURSE ALL_SOURCE_FILES
        CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/test/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/example/pdevs/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/example/devs/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/example/stdevs/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/example/qss/*.cpp"
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
