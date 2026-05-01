# cadmium [![License](https://img.shields.io/badge/License-BSD%202--Clause-orange.svg)](https://opensource.org/licenses/BSD-2-Clause)

This is **sdavtaker's fork** of [SimulationEverywhere/cadmium](https://github.com/SimulationEverywhere/cadmium),
modernized to C++23 for use in the VDW14/VDW16/VWT19 time-representation experiments.
It is not intended for general distribution — changes stay on this fork only.

## What this fork is

A header-only C++23 PDEVS simulator. Keeps the static and dynamic PDEVS engines and
the classic basic models. Removes Cell-DEVS, real-time support, async/concurrent modes,
and all Boost dependencies.

### What is kept

- Static PDEVS engine: `engine/pdevs_simulator.hpp`, `pdevs_coordinator.hpp`,
  `pdevs_runner.hpp`, `pdevs_engine_helpers.hpp`
- Dynamic PDEVS engine: `engine/pdevs_dynamic_*.hpp`
- Basic models: `basic_model/devs/` and `basic_model/pdevs/`
- Modeling: `modeling/coupling.hpp`, `message_bag.hpp`, `message_box.hpp`, `ports.hpp`,
  `dynamic_*.hpp`
- Logger: spdlog-based NDJSON logger (`logger/cadmium_log.hpp`)
- I/O: `io/iestream.hpp`, `io/oestream.hpp`
- Examples: `example/pdevs/`

### What was removed

- `celldevs/` — Cell-DEVS extension
- `real_time/` — ARM mbed / Linux / Windows real-time clocks
- `engine/pdevs_dynamic_asynchronus_simulator.hpp` — async mode
- `engine/concurrency_helpers.hpp` / `parallel_helpers.hpp` — concurrent/parallel modes
- All Boost dependencies
- BJam build system
- Git submodules (`DESTimes/`, `cmake-modules/`, `json/`)

## Requirements

| Tool | Minimum version |
|---|---|
| g++ | 14 (GCC 13 breaks C++23 concepts) |
| CMake | 3.28 |
| vcpkg | any recent |

Dependencies are managed via vcpkg: `spdlog` and `catch2`.

## Building

```sh
# one-time: bootstrap vcpkg if not already done
git clone https://github.com/microsoft/vcpkg "$VCPKG_ROOT"
"$VCPKG_ROOT/bootstrap-vcpkg.sh"

cmake -S . -B build \
  -DCMAKE_CXX_COMPILER=g++-14 \
  -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"

cmake --build build
```

## Building and running tests

```sh
cmake -S . -B build \
  -DCMAKE_CXX_COMPILER=g++-14 \
  -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" \
  -DCADMIUM_BUILD_TESTS=ON

cmake --build build
ctest --test-dir build
```

## Format check

```sh
cmake --build build --target check-format   # report violations
cmake --build build --target format         # apply fixes
```

## Defining a PDEVS model

```cpp
#include <cadmium/modeling/ports.hpp>
#include <cadmium/modeling/message_bag.hpp>

struct OutPort : public cadmium::out_port<int> {};

template <typename TIME>
struct Counter {
    using input_ports  = std::tuple<>;
    using output_ports = std::tuple<OutPort>;

    struct state_type { int n = 0; };
    state_type state;

    TIME time_advance() const { return TIME{1}; }
    void internal_transition() { state.n++; }
    void external_transition(TIME, typename cadmium::make_message_bags<input_ports>::type) {}
    void confluence_transition(TIME e, typename cadmium::make_message_bags<input_ports>::type b) {
        internal_transition();
        external_transition(TIME{0}, b);
    }
    typename cadmium::make_message_bags<output_ports>::type output() const {
        auto bags = typename cadmium::make_message_bags<output_ports>::type{};
        cadmium::get_messages<OutPort>(bags).push_back(state.n);
        return bags;
    }
};
```

## Running a simulation

```cpp
#include <cadmium/engine/pdevs_runner.hpp>
#include <cadmium/logger/cadmium_log.hpp>

int main() {
    cadmium::log::init();   // enable NDJSON logging to stdout
    cadmium::engine::runner<float, Counter> r{0.0f};
    r.run_until(10.0f);
}
```

The runner also accepts an atomic model directly (no coupled wrapper needed):
```cpp
cadmium::engine::runner<float, Counter> r{0.0f};   // atomic at top level
```

## Logger

The logger emits NDJSON lines to stdout. Each line has the fields:
`ts`, `level`, `event`, `msg`, and optionally `sim_time`.

```json
{"ts":"2026-04-30T14:00:00Z","level":"info","event":"run_info","msg":"Starting run"}
{"ts":"2026-04-30T14:00:00Z","level":"debug","event":"sim_state","msg":"counter 0","sim_time":1.0}
```

Log levels: `debug`, `info`, `warn`, `error`. Engine errors (time-scope violations,
invalid coupling) are logged at `error` before throwing. The runner emits `warn` when
the simulation passivates before reaching the requested end time.

Call `cadmium::log::init()` once before constructing the runner to activate logging.
Without it, all `emit()` calls are no-ops.

## References

- [Sequential PDEVS architecture (VNWD15)](http://cell-devs.sce.carleton.ca/publications/2015/VNWD15/)
- [Building DEVS Models with Cadmium (VWRB19)](https://doi.org/10.1109/WSC48552.2020.9383872)
- [upstream SimulationEverywhere/cadmium](https://github.com/SimulationEverywhere/cadmium)
