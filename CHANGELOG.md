# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

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

[Unreleased]: https://github.com/sdavtaker/cadmium/compare/v0.3.0...HEAD
[0.3.0]: https://github.com/sdavtaker/cadmium/compare/v0.2.9...v0.3.0
[0.2.9]: https://github.com/sdavtaker/cadmium/releases/tag/v0.2.9
