# PDEVS in cadmium

Parallel DEVS (PDEVS) extends classic DEVS so that multiple models may be imminent
at the same logical time without SELECT serialisation.  Inputs arrive as *bags* —
multisets that can carry multiple messages on a single port in one time step — and a
*confluence* function δ_con handles the case where an external input arrives exactly
when an internal transition is scheduled.

Formal treatment: Chow & Zeigler — *Parallel DEVS: A Common Framework for
Time-Based and Event-Based Simulation* (SCS Multiconference, 1994);
Vicino, Niyonkuru, Wainer, Dalle — *Sequential PDEVS Architecture* (SpringSim, 2015).

---

## Formalism → cadmium mapping

| Formal element | C++ construct |
|---|---|
| X (input value set) | `message_type` on `cadmium::in_port<T>` |
| Y (output value set) | `message_type` on `cadmium::out_port<T>` |
| S (state set) | nested `state_type` struct; public `state` member |
| δ_int(s) | `void internal_transition()` |
| δ_ext(s, e, X_b) | `void external_transition(TIME elapsed, message_bags in_bags)` |
| δ_con(s, X_b) | `void confluence_transition(TIME elapsed, message_bags in_bags)` |
| λ(s) | `message_bags output() const` |
| ta(s) | `TIME time_advance() const` |
| Coupled model | `cadmium::modeling::pdevs::coupled_model<…>` (no SELECT) |
| Root coordinator | `cadmium::engine::runner<TIME, MODEL>` |

Each port carries a **bag** — a `std::vector<T>` — so multiple messages may arrive
on the same port in one step.  There is no SELECT: all imminent submodels fire in
parallel.

---

## Implementing an atomic model

Include path: `<cadmium/modeling/message_bag.hpp>`, `<cadmium/modeling/ports.hpp>`

```cpp
#include <cadmium/modeling/message_bag.hpp>
#include <cadmium/modeling/ports.hpp>
#include <limits>
#include <ostream>

// ── Port declarations ─────────────────────────────────────────────────────────
// Message types must support operator<< (required by cadmium::concepts::Streamable).

struct Reset {
    friend std::ostream& operator<<(std::ostream& os, const Reset&) {
        return os << "Reset{}";
    }
};

struct AddPort   : public cadmium::in_port<int>   {};
struct ResetPort : public cadmium::in_port<Reset>  {};
struct SumPort   : public cadmium::out_port<int>  {};

// ── Atomic model ──────────────────────────────────────────────────────────────

template <typename TIME>
struct Accumulator {
    using input_ports  = std::tuple<AddPort, ResetPort>;
    using output_ports = std::tuple<SumPort>;

    struct state_type {
        int  total{0};
        bool reset_pending{false};
        friend std::ostream& operator<<(std::ostream& os, const state_type& s) {
            return os << "total=" << s.total;
        }
    };
    state_type state;

    TIME time_advance() const {
        if (state.reset_pending)
            return TIME{0};   // fire immediately to emit the sum
        return std::numeric_limits<TIME>::infinity();
    }

    void internal_transition() {
        state.total         = 0;
        state.reset_pending = false;
    }

    void external_transition(TIME /*elapsed*/,
                             typename cadmium::make_message_bags<input_ports>::type in_bags) {
        // Process all add messages (bag may contain many)
        for (int v : cadmium::get_messages<AddPort>(in_bags))
            state.total += v;

        // Any reset message schedules immediate output
        if (!cadmium::get_messages<ResetPort>(in_bags).empty())
            state.reset_pending = true;
    }

    // δ_con: input arrives exactly when ta(s) would fire.
    // Canonical choice: do internal first, then external at elapsed=0.
    void confluence_transition(TIME elapsed,
                               typename cadmium::make_message_bags<input_ports>::type in_bags) {
        internal_transition();
        external_transition(TIME{0}, in_bags);
    }

    typename cadmium::make_message_bags<output_ports>::type output() const {
        auto bags = typename cadmium::make_message_bags<output_ports>::type{};
        cadmium::get_messages<SumPort>(bags).push_back(state.total);
        return bags;
    }
};
```

### Reading from a bag

```cpp
for (const auto& v : cadmium::get_messages<MyInPort>(in_bags)) {
    // v is of type MyInPort::message_type
}
```

`cadmium::get_messages<PORT>(bags)` returns `std::vector<T>&`.

### Writing to a bag

```cpp
cadmium::get_messages<MyOutPort>(bags).push_back(my_value);
```

### Passive state

Return `std::numeric_limits<TIME>::infinity()` from `time_advance()`.  The model
will not fire again until it receives an external event.

### Confluence convention

The most common confluence is *internal-first*: emit output, do δ_int, then treat
the arriving inputs as if they arrived at elapsed = 0.  Any other semantics are
valid as long as they produce a well-defined next state.

---

## Running an atomic model

```cpp
#include <cadmium/engine/pdevs_runner.hpp>
#include <cadmium/logger/cadmium_log.hpp>

int main() {
    cadmium::log::init();   // optional: emit NDJSON events to stdout

    cadmium::engine::runner<float, Accumulator> r{0.0f};
    r.run_until(100.0f);
    // or: r.run_until_passivate();
}
```

---

## Implementing a coupled model

```cpp
#include <cadmium/modeling/coupling.hpp>
#include <cadmium/modeling/ports.hpp>

// Coupled-model output port
struct TopSumOut : public cadmium::out_port<int> {};

// Submodel list
using submodels = cadmium::modeling::models_tuple<Accumulator>;

// Couplings (no external inputs in this example)
using eics = std::tuple<>;
using eocs = std::tuple<
    cadmium::modeling::EOC<Accumulator, SumPort, TopSumOut>
>;
using ics  = std::tuple<>;

// Coupled model type — no SELECT needed for PDEVS
template <typename TIME>
using TopModel = cadmium::modeling::pdevs::coupled_model<
    TIME,
    std::tuple<>,          // input_ports
    std::tuple<TopSumOut>, // output_ports
    submodels,
    eics, eocs, ics
>;
```

### Coupling entry types

| Type | Connects |
|---|---|
| `EIC<ExternalPort, Submodel, SubmodelPort>` | coupled input → submodel input |
| `EOC<Submodel, SubmodelPort, ExternalPort>` | submodel output → coupled output |
| `IC<FromModel, FromPort, ToModel, ToPort>` | submodel output → submodel input |

---

## Running a coupled model

```cpp
cadmium::engine::runner<float, TopModel> r{0.0f};
r.run_until(100.0f);
```

---

## Available basic models

| Header | Model | Behaviour |
|---|---|---|
| `<cadmium/basic_model/pdevs/generator.hpp>` | `generator<Derived, MSG, TIME>` | CRTP periodic event source; override `period()` and `output_message()` |
| `<cadmium/basic_model/pdevs/accumulator.hpp>` | `accumulator<T, TIME>` | sums values on `add` bag; emits and resets on `reset` bag |
| `<cadmium/basic_model/pdevs/int_generator_one_sec.hpp>` | `int_generator_one_sec` | emits 1 every second |
| `<cadmium/basic_model/pdevs/reset_generator_five_sec.hpp>` | `reset_generator_five_sec` | emits a reset tick every 5 seconds |
| QSS models: `<cadmium/basic_model/qss/>` | integrators, wsum, multiplier, … | quantized-state continuous integration |

---

## Further reading

- `example/pdevs/main-clock.cpp` — three-generator clock with the coupled runner
- `example/pdevs/main-count-fives.cpp` — generator + accumulator in nested coupled models
- [cadmium-experiments](https://github.com/sdavtaker/cadmium-experiments) — VDW14 PDEVS
  tick-counter experiment; QSS1 bouncing ball and NCS hybrid simulation
