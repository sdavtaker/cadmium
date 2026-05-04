# cadmium Log Format

cadmium emits structured **NDJSON** (Newline-Delimited JSON) — one JSON object per line.
Output goes to stdout via spdlog. Call `cadmium::log::init()` once at program startup to
activate logging; without it, all `emit()` calls are no-ops.

## Required Fields

Every log record contains exactly these fields:

| Field   | Type   | Description |
|---------|--------|-------------|
| `ts`    | string | Wall-clock timestamp, ISO 8601 UTC with millisecond precision: `2026-04-27T14:05:32.417Z` |
| `level` | string | Severity: `debug`, `info`, `warn`, or `error` |
| `event` | string | Structured event name (see Event Names below) |
| `msg`   | string | Human-readable description of the event |

## Optional Fields

| Field      | Type   | Description |
|------------|--------|-------------|
| `sim_time` | number | Simulation time as a double-precision float. **Never replaces `ts`** — `ts` is always wall-clock time. `sim_time` carries the logical simulation time. |

## ts vs sim_time

`ts` is the wall-clock instant the log line was written. `sim_time` is the logical time inside
the simulation. They measure different things and are always independent.

- A simulation running 30000 logical seconds may complete in 0.1 wall-clock seconds.
- Use `ts` to correlate log lines with external events (CI timestamps, profiling).
- Use `sim_time` to understand where in the simulation an event occurred.

## Event Names

### Runner Events

Emitted by `cadmium::engine::runner` (both the static and dynamic variants).

| Event            | Level  | sim_time | msg |
|------------------|--------|----------|-----|
| `run_global_time`| info   | yes (initial time) | `"start"` — emitted once at initialization |
| `run_info`       | info   | yes (initial time) | `"Preparing model"` — emitted before subengine init |
| `run_info`       | info   | no | `"Starting run"` — emitted at the top of `run_until` |
| `run_global_time`| debug  | yes (next event time) | `"step"` — emitted once per simulation step |
| `run_info`       | warn   | no | `"simulation passivated before reaching end time"` — emitted when `_next == infinity` at the end of `run_until` |
| `run_info`       | info   | no | `"Finished run"` — emitted at the end of `run_until` |

### Coordinator Events

Emitted by `cadmium::engine::coordinator` (both static and dynamic variants).
The `msg` field always includes the model ID as a prefix.

| Event                    | Level  | sim_time | Description |
|--------------------------|--------|----------|-------------|
| `coor_info_init`         | info   | yes | Coordinator initialized. `msg` = model ID. |
| `coor_info_collect`      | debug  | yes | `collect_outputs` called. |
| `coor_routing_eoc_collect`| debug | yes | EOC routing phase of `collect_outputs` (only when `t == _next`). |
| `coor_routing_ic_collect` | debug | yes | IC routing phase of `advance_simulation`. |
| `coor_routing_eic_collect`| debug | yes | EIC routing phase of `advance_simulation`. |
| `coor_info_advance`      | debug  | yes | `advance_simulation` called. |
| `coor_error`             | error  | yes | Protocol violation. `msg` includes the specific failure: `"collect_outputs called past next event"`, `"advance_simulation called outside time scope"`, `"EOC from unknown model <id>"`, `"EIC to unknown model <id>"`, `"IC references unknown model(s) <from> -> <to>"`. |

### Simulator Events

Emitted by `cadmium::engine::simulator` (both static and dynamic variants).
The `msg` field always includes the model ID as a prefix.

| Event                  | Level  | sim_time | Description |
|------------------------|--------|----------|-------------|
| `sim_info_init`        | info   | yes | Simulator initialized. `msg` = model ID. |
| `sim_state`            | debug  | yes | State snapshot. `msg` = `"<model_id> <state_ostream>"`. Emitted after init and after every `advance_simulation`. |
| `sim_info_collect`     | debug  | yes | `collect_outputs` called. |
| `sim_messages_collect` | debug  | yes | Output collected. `msg` = `"<model_id> <port>: [<msg>, ...]"`. Emitted every time `collect_outputs` runs (empty string when `t != _next`). |
| `sim_info_advance`     | debug  | yes | `advance_simulation` called. |
| `sim_error`            | error  | yes | Protocol violation. `msg` includes the specific failure: `"collect_outputs called past next event"`, `"advance_simulation called in the past"`, `"advance_simulation called past next scheduled event"`. |

## Example Output

A two-model PDEVS simulation with a generator and a counter:

```ndjson
{"ts":"2026-04-27T14:05:32.000Z","level":"info","event":"run_global_time","msg":"start","sim_time":0.0}
{"ts":"2026-04-27T14:05:32.001Z","level":"info","event":"run_info","msg":"Preparing model","sim_time":0.0}
{"ts":"2026-04-27T14:05:32.001Z","level":"info","event":"coor_info_init","msg":"top","sim_time":0.0}
{"ts":"2026-04-27T14:05:32.001Z","level":"info","event":"sim_info_init","msg":"generator","sim_time":0.0}
{"ts":"2026-04-27T14:05:32.001Z","level":"debug","event":"sim_state","msg":"generator 0","sim_time":0.0}
{"ts":"2026-04-27T14:05:32.001Z","level":"info","event":"sim_info_init","msg":"counter","sim_time":0.0}
{"ts":"2026-04-27T14:05:32.001Z","level":"debug","event":"sim_state","msg":"counter 0","sim_time":0.0}
{"ts":"2026-04-27T14:05:32.002Z","level":"info","event":"run_info","msg":"Starting run"}
{"ts":"2026-04-27T14:05:32.002Z","level":"debug","event":"run_global_time","msg":"step","sim_time":1.0}
{"ts":"2026-04-27T14:05:32.002Z","level":"debug","event":"coor_info_collect","msg":"top","sim_time":1.0}
{"ts":"2026-04-27T14:05:32.002Z","level":"debug","event":"coor_routing_eoc_collect","msg":"top","sim_time":1.0}
{"ts":"2026-04-27T14:05:32.002Z","level":"debug","event":"sim_info_collect","msg":"generator","sim_time":1.0}
{"ts":"2026-04-27T14:05:32.002Z","level":"debug","event":"sim_messages_collect","msg":"generator out: [tick]","sim_time":1.0}
{"ts":"2026-04-27T14:05:32.003Z","level":"debug","event":"coor_info_advance","msg":"top","sim_time":1.0}
{"ts":"2026-04-27T14:05:32.003Z","level":"debug","event":"coor_routing_ic_collect","msg":"top","sim_time":1.0}
{"ts":"2026-04-27T14:05:32.003Z","level":"debug","event":"coor_routing_eic_collect","msg":"top","sim_time":1.0}
{"ts":"2026-04-27T14:05:32.003Z","level":"debug","event":"sim_info_advance","msg":"generator","sim_time":1.0}
{"ts":"2026-04-27T14:05:32.003Z","level":"debug","event":"sim_state","msg":"generator 1","sim_time":1.0}
{"ts":"2026-04-27T14:05:32.003Z","level":"debug","event":"sim_info_advance","msg":"counter","sim_time":1.0}
{"ts":"2026-04-27T14:05:32.003Z","level":"debug","event":"sim_state","msg":"counter 1","sim_time":1.0}
{"ts":"2026-04-27T14:05:32.100Z","level":"info","event":"run_info","msg":"Finished run"}
```

## Filtering with jq

Show only the simulation steps (one line per event):
```sh
jq 'select(.event == "run_global_time" and .msg == "step")' sim.ndjson
```

Extract every state snapshot as `model → state`:
```sh
jq -r 'select(.event == "sim_state") | .msg' sim.ndjson
```

Show all error events:
```sh
jq 'select(.level == "error")' sim.ndjson
```

Count total simulation steps:
```sh
jq -s '[.[] | select(.event == "run_global_time" and .msg == "step")] | length' sim.ndjson
```

Show simulation time range (first and last step):
```sh
jq -r 'select(.event == "run_global_time") | [.msg, (.sim_time // "n/a") | tostring] | join(" ")' sim.ndjson
```

## C++ API Reference

```cpp
#include <cadmium/logger/cadmium_log.hpp>

// Initialize once at startup. No-op if already initialized.
cadmium::log::init();

// Emit a log record. sim_time is optional.
cadmium::log::emit(cadmium::log::level::info, "event_name", "message");
cadmium::log::emit(cadmium::log::level::debug, "sim_state", "my_model idle", 1.0);

// Flush pending records to stdout.
cadmium::log::flush();

// Log an exception at error level and flush (call inside catch block before rethrow).
cadmium::log::log_exception(e, "simulation_error");

// Convert a simulation TIME value to double for use as sim_time.
// Specialize for types where static_cast<double> is insufficient.
double t = cadmium::log::to_sim_double(my_time_value);
```

## Debug vs Info vs Warn vs Error

- **debug**: Per-step routing and state details. Suppress with a log-level filter if only interested in high-level progress.
- **info**: Lifecycle milestones (start, init, finish) and model state at init time.
- **warn**: Unexpected but recoverable: simulation passivated before the requested end time.
- **error**: Protocol violations (time ordering errors, unknown coupling targets). Always followed by an exception.
