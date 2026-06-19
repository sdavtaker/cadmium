# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [0.4.0] - 2026-06-19

### Added
- `Streamable` concept in `common_concepts.hpp`: satisfied when `os << v` is valid for a type.
- `all_message_types_streamable_v` helper: true when every port in a tuple has a streamable `message_type`.
- `Streamable` requirements added to `AtomicModel` in `pdevs_concepts.hpp`, `stdevs_concepts.hpp`, and `devs_concepts.hpp` — state type and all port message types must be streamable.
- Inbox logging in `pdevs_simulator` and `stdevs_simulator`: a `sim_messages_advance` / `stdev_sim_messages_advance` NDJSON event is emitted before each external or confluence transition.
- QSS1 atomic models: `qss1_integrator<T>`, `qss_wsum<N,T>`, `qss_multiplier<T>` in `include/cadmium/basic_model/qss/`.
- Time-independent port defs structs for QSS atomics: `qss1_integrator_defs`, `qss_multiplier_defs`, `qss_wsum_defs<N>`. All QSS template instantiations share the same port types regardless of `TIME`.
- Lotka-Volterra QSS1 example (`example/qss/main-lotka-volterra.cpp`) as a proper `devs::CoupledModel` driven by `cadmium::engine::devs::runner`.
- QSS1 Lotka-Volterra integration test (`test/qss1_lotka_volterra_test.cpp`) validating populations stay positive, conserved-quantity drift is small, and convergence under finer quantum.

### Changed
- **Breaking**: `AtomicModel` concepts in all three engine families now require `operator<<` on `state_type` and on every port's `message_type`. Custom atomic models must add streaming support.
- `devs::accumulator` state type converted from `std::tuple<VALUE, bool>` to a named struct (`sum`, `on_reset`) with `operator<<`; `reset_tick` gained `operator<<`.
- `pdevs::accumulator` state type similarly converted to a named struct (done in 0.4.0 preparation).
- DEVS and PDEVS test helpers: `tuple_to_ostream` includes removed; `tick`/`test_tick` message types gained `operator<<`.

## [0.3.1] - 2026-06-04

### Fixed
- CMake install configuration now generates a proper `cadmiumConfig.cmake` using
  `configure_package_config_file` with `find_dependency(spdlog)`, so
  `find_package(cadmium)` works correctly after vcpkg installation without
  consumers needing to separately find spdlog.
- `write_basic_package_version_file` compatibility changed to `SameMinorVersion`
  (was `AnyNewerVersion`), matching pre-1.0 SemVer semantics.

## [0.3.0] - 2026-05-02

C++23 modernization fork — stripped to a clean PDEVS engine, Boost removed,
structured logging added.

### Added
- NDJSON structured logging via spdlog; all engine events emit machine-readable JSON lines
- Error and warn log levels to the PDEVS engine alongside the existing info/debug levels
- Typed `outbox_port<Port>()` and `inbox_port<Port>()` accessors on the coordinator
- Runner now accepts an atomic model directly at the top level (no forced wrapper coupling)
- Catch2 unit test suite covering coordinator, simulator, and concept checks
- `check-cadmium.sh` build-and-test validation script
- GitHub Actions CI workflow (GCC 14, vcpkg, C++23)
- CMake C++23 build system with vcpkg dependency management

### Changed
- `std::any` / `std::type_index` replace `boost::any` / `boost::typeindex`
- LOGGER template parameter removed from engine; logging always uses spdlog NDJSON
- Build migrated from BJam to CMake; C++ standard raised to 23
- `concept` sub-namespace renamed to `model_concept` to avoid the C++20 keyword
- Compile-fail test harness replaced with runtime `STATIC_REQUIRE` checks

### Removed
- Boost dependency (Boost.Any, Boost.TypeIndex, Boost.Test, BJam)
- Cell-DEVS, RT-DEVS, concurrent/async simulation engines
- Legacy submodules: DESTimes, cmake-modules, json
- All `#ifdef` guards for disabled engine variants

## [0.2.9] - 2024-02-01

Last upstream release from SimulationEverywhere/cadmium before this fork.

[Unreleased]: https://github.com/sdavtaker/cadmium/compare/v0.4.0...HEAD
[0.4.0]: https://github.com/sdavtaker/cadmium/compare/v0.3.1...v0.4.0
[0.3.1]: https://github.com/sdavtaker/cadmium/compare/v0.3.0...v0.3.1
[0.3.0]: https://github.com/sdavtaker/cadmium/compare/v0.2.9...v0.3.0
[0.2.9]: https://github.com/sdavtaker/cadmium/releases/tag/v0.2.9
