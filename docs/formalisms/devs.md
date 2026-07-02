# Classic DEVS in cadmium

Classic DEVS (Discrete Event System Specification) decomposes a model into atomic
components, each with a well-defined lifecycle: an autonomous internal transition
fires after a scheduled time advance, and an external transition fires whenever an
input arrives.  Coupled models wire atomics together; a SELECT function serialises
simultaneous events across submodels.

Formal treatment: Zeigler, Praehofer, Kim —
*Theory of Modeling and Simulation* (Academic Press, 2000).

---

## Formalism → cadmium mapping

| Formal element | C++ construct |
|---|---|
| X (input value set) | `message_type` on `cadmium::in_port<T>` |
| Y (output value set) | `message_type` on `cadmium::out_port<T>` |
| S (state set) | nested `state_type` struct; public `state` member |
| δ_int(s) | `void internal_transition()` |
| δ_ext(s, e, x) | `void external_transition(TIME elapsed, message_box in_box)` |
| λ(s) | `message_box output() const` |
| ta(s) | `TIME time_advance() const` |
| Coupled model | `cadmium::modeling::devs::coupling<…, SELECT>` |
| SELECT | struct with `static size_t apply(engines, t)` |
| Root coordinator | `cadmium::engine::devs::runner<TIME, MODEL>` |

Each port carries **at most one message at a time** (`std::optional<T>` inside the
box).  There is no confluence function — simultaneous events are serialised by SELECT.

---

## Implementing an atomic model

Include path: `<cadmium/modeling/message_box.hpp>`, `<cadmium/modeling/ports.hpp>`

```cpp
#include <cadmium/modeling/message_box.hpp>
#include <cadmium/modeling/ports.hpp>
#include <limits>
#include <ostream>

// ── Port declarations ─────────────────────────────────────────────────────────
// Message types must support operator<< (required by cadmium::concepts::Streamable).

struct Token {
    int id;
    friend std::ostream& operator<<(std::ostream& os, const Token& t) {
        return os << "Token{" << t.id << "}";
    }
};

struct InPort  : public cadmium::in_port<Token>  {};
struct OutPort : public cadmium::out_port<Token> {};

// ── Atomic model ──────────────────────────────────────────────────────────────

template <typename TIME>
struct DelayLine {
    // Required type aliases
    using input_ports  = std::tuple<InPort>;
    using output_ports = std::tuple<OutPort>;

    // State — must support operator<<
    struct state_type {
        Token   held{0};
        bool    active{false};
        TIME    remaining{0};
        friend std::ostream& operator<<(std::ostream& os, const state_type& s) {
            return os << "active=" << s.active << " id=" << s.held.id;
        }
    };
    state_type state;

    // ta(s): return time until next autonomous event; infinity = passive
    TIME time_advance() const {
        if (state.active)
            return state.remaining;
        return std::numeric_limits<TIME>::infinity();
    }

    // δ_int(s): autonomous transition — fires after ta(s) elapses
    void internal_transition() {
        state.active    = false;
        state.remaining = std::numeric_limits<TIME>::infinity();
    }

    // δ_ext(s, e, x): input arrives; e = elapsed time since last transition
    void external_transition(TIME elapsed,
                             typename cadmium::make_message_box<input_ports>::type in_box) {
        if (!state.active) {
            if (auto msg = cadmium::get_message<InPort>(in_box)) {
                state.held      = *msg;
                state.active    = true;
                state.remaining = TIME{1};  // fixed 1-time-unit delay
            }
        } else {
            // already holding a token — update remaining and ignore new arrival
            state.remaining -= elapsed;
        }
    }

    // λ(s): output produced just before δ_int fires
    typename cadmium::make_message_box<output_ports>::type output() const {
        auto box = typename cadmium::make_message_box<output_ports>::type{};
        cadmium::get_message<OutPort>(box) = state.held;
        return box;
    }
};
```

### Reading a message from the input box

```cpp
if (auto msg = cadmium::get_message<MyInPort>(in_box)) {
    // *msg is the value (type = MyInPort::message_type)
}
```

`cadmium::get_message<PORT>(box)` returns `std::optional<T>`.  An empty optional
means no message arrived on that port this step.

### Writing a message to the output box

```cpp
cadmium::get_message<MyOutPort>(box) = my_value;
```

---

## Running an atomic model

```cpp
#include <cadmium/engine/devs_runner.hpp>
#include <cadmium/logger/cadmium_log.hpp>

int main() {
    cadmium::log::init();   // optional: emit NDJSON events to stdout

    cadmium::engine::devs::runner<float, DelayLine> r{0.0f};
    r.run_until(100.0f);    // stop at t=100
    // or: r.run_until_passivate();  // stop when ta=∞
}
```

The runner owns a `DelayLine<float>` and drives it via the DEVS abstract simulator
protocol.  Every time step:
1. `output()` is called on the imminent model.
2. Its output is routed to any receivers.
3. `internal_transition()` fires on the imminent model;
   `external_transition()` fires on every receiver.

---

## Implementing a coupled model

A coupled model is a pure type — no runtime members.  It lists submodels and the
couplings between them.

```cpp
#include <cadmium/engine/devs_engine_helpers.hpp>  // first_imminent
#include <cadmium/modeling/coupling.hpp>
#include <cadmium/modeling/ports.hpp>

// External output port of the coupled model
struct CoupledOut : public cadmium::out_port<Token> {};

// Submodels (template-on-TIME wrappers)
using my_submodels = cadmium::modeling::models_tuple<DelayLine>;  // one submodel

// Couplings
using my_eics = std::tuple<>;   // no external inputs in this example
using my_eocs = std::tuple<
    cadmium::modeling::EOC<DelayLine, OutPort, CoupledOut>
>;
using my_ics  = std::tuple<>;   // no internal connections

// Coupled model type — SELECT = first_imminent (pick lowest tuple index)
template <typename TIME>
using MyCoupled = cadmium::modeling::devs::coupling<
    TIME,
    std::tuple<>,       // input_ports of the coupled model
    std::tuple<CoupledOut>,
    my_submodels,
    my_eics,
    my_eocs,
    my_ics,
    cadmium::engine::devs::first_imminent
>;
```

### Coupling entry types

| Type | Connects |
|---|---|
| `EIC<ExternalPort, Submodel, SubmodelPort>` | coupled input → submodel input |
| `EOC<Submodel, SubmodelPort, ExternalPort>` | submodel output → coupled output |
| `IC<FromModel, FromPort, ToModel, ToPort>` | submodel output → submodel input |

### SELECT

`cadmium::engine::devs::first_imminent` is the built-in SELECT that picks the
submodel with the lowest index in the models tuple when multiple fire at the same
time.  To define a custom priority, provide a struct with:

```cpp
struct MySelect {
    template <typename Engines, typename TIME>
    static std::size_t apply(const Engines& engines, TIME t_n);
};
```

---

## Running a coupled model

```cpp
cadmium::engine::devs::runner<float, MyCoupled> r{0.0f};
r.run_until(100.0f);
```

---

## Available basic models

| Header | Model | Behaviour |
|---|---|---|
| `<cadmium/basic_model/devs/generator.hpp>` | `generator<Derived, MSG, TIME>` | CRTP periodic event source; override `period()` and `output_message()` |
| `<cadmium/basic_model/devs/accumulator.hpp>` | `accumulator<T, TIME>` | sums values on `add` port; emits sum and resets on `reset` port |
| `<cadmium/basic_model/devs/passive.hpp>` | `passive<TIME>` | always passive; useful as a sink or stub |

---

## Further reading

- `example/devs/main-clock.cpp` — three generators with explicit SELECT loop
- `example/devs/main-count-fives.cpp` — generator + accumulator with SELECT loop
- [cadmium-experiments](https://github.com/sdavtaker/cadmium-experiments) — VDW14
  tick-counter and QSS1 ball-downstairs experiments using classic DEVS atomics
