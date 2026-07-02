# STDEVS in cadmium

STDEVS (Stochastic DEVS) extends classic DEVS by replacing deterministic transition
functions with probability-space-valued mappings.  The key insight — proven in the
original paper — is that any such mapping is equivalent to a sampling function that
takes a stream of uniform random numbers and returns the next state.  In cadmium this
is realised by passing a `URNG&` directly into the internal and external transition
functions: the user draws from any `std::<random>` distribution inside the function
body.

The output function λ and time-advance ta must remain **deterministic** (they are
required to be measurable functions of state, per CKW10 Section 4.2).

Coupling structure is identical to classic DEVS: the same port system, EIC/EOC/IC
entries, and SELECT tie-breaking function are used.

Formal treatment: Castro, Kofman, Wainer —
*A Formal Framework for Stochastic Discrete Event System Specification Modeling
and Simulation*, SIMULATION 86(10), 2010 (DOI 10.1177/0037549709104482).
Conference precursor: SpringSim 2008.

---

## Formalism → cadmium mapping

| Formal element | C++ construct |
|---|---|
| X (input value set) | `message_type` on `cadmium::in_port<T>` |
| Y (output value set) | `message_type` on `cadmium::out_port<T>` |
| S (state set) | nested `state_type` struct; public `state` member |
| G_int / P_int (internal stochastic mapping) | `void internal_transition(URNG& rng)` |
| G_ext / P_ext (external stochastic mapping) | `void external_transition(TIME elapsed, message_box in_box, URNG& rng)` |
| λ(s) — deterministic | `message_box output() const` |
| ta(s) — deterministic | `TIME time_advance() const` |
| Coupled model | `cadmium::modeling::devs::coupling<…, SELECT>` (same as DEVS) |
| Root coordinator | `cadmium::engine::stdevs::runner<TIME, MODEL, URNG>` |

The URNG is owned by the root coordinator and passed down; each submodel sees the
same generator in sequence, so simulation is **exactly reproducible** given the seed.

---

## Implementing an atomic model

Include path: `<cadmium/modeling/message_box.hpp>`, `<cadmium/modeling/ports.hpp>`

```cpp
#include <cadmium/modeling/message_box.hpp>
#include <cadmium/modeling/ports.hpp>
#include <limits>
#include <ostream>
#include <random>

// ── Port declarations ─────────────────────────────────────────────────────────

struct Job {
    int id;
    friend std::ostream& operator<<(std::ostream& os, const Job& j) {
        return os << "Job{" << j.id << "}";
    }
};

struct InPort  : public cadmium::in_port<Job>  {};
struct OutPort : public cadmium::out_port<Job> {};

// ── Atomic model: M/M/1/1 server (single-buffer exponential service) ──────────
//
// Accepts a job when idle; serves it for Exp(mu) time; then outputs and becomes
// idle again.  A job arriving while busy is dropped.

template <typename TIME>
struct ExponentialServer {
    using input_ports  = std::tuple<InPort>;
    using output_ports = std::tuple<OutPort>;

    struct state_type {
        Job   current{0};
        bool  busy{false};
        TIME  sigma{std::numeric_limits<TIME>::infinity()};
        friend std::ostream& operator<<(std::ostream& os, const state_type& s) {
            return os << "busy=" << s.busy << " id=" << s.current.id;
        }
    };
    state_type state;

    const double mu;   // service rate (events/time unit)
    explicit ExponentialServer(double service_rate) : mu(service_rate) {}

    // ta(s): deterministic — how long until service completes
    TIME time_advance() const { return state.sigma; }

    // δ_int(s, rng): service complete — output already sent; become idle
    void internal_transition(std::mt19937& /*rng*/) {
        state.busy  = false;
        state.sigma = std::numeric_limits<TIME>::infinity();
    }

    // δ_ext(s, e, x, rng): job arrives — accept if idle, drop if busy
    void external_transition(TIME /*elapsed*/,
                             typename cadmium::make_message_box<input_ports>::type in_box,
                             std::mt19937& rng) {
        if (state.busy)
            return;  // single-buffer: drop arriving job

        if (auto msg = cadmium::get_message<InPort>(in_box)) {
            state.current = *msg;
            state.busy    = true;
            // Draw service time from Exp(mu)
            std::exponential_distribution<double> exp_dist(mu);
            state.sigma = static_cast<TIME>(exp_dist(rng));
        }
    }

    // λ(s): deterministic output — deliver the completed job
    typename cadmium::make_message_box<output_ports>::type output() const {
        auto box = typename cadmium::make_message_box<output_ports>::type{};
        cadmium::get_message<OutPort>(box) = state.current;
        return box;
    }
};
```

### Rules for λ and ta

Both functions receive no `rng` argument and must be **pure functions of state**.
Any randomness must be committed to state during a transition (δ_int or δ_ext)
before λ or ta inspect it.

For example, if the output message depends on a random draw, compute it during
δ_int and store it in state; λ then reads the stored value.

### Passive state

Return `std::numeric_limits<TIME>::infinity()` from `time_advance()`.

---

## Running an atomic model

```cpp
#include <cadmium/engine/stdevs_runner.hpp>
#include <cadmium/logger/cadmium_log.hpp>
#include <random>

int main() {
    cadmium::log::init();   // optional: NDJSON events to stdout

    const auto seed = std::mt19937::result_type{42};

    // Seeded constructor: fully reproducible
    cadmium::engine::stdevs::runner<double, ExponentialServer> r{0.0, seed};
    r.run_until(1000.0);
    // or: r.run_until_passivate();

    // Non-deterministic constructor: uses std::random_device
    // cadmium::engine::stdevs::runner<double, ExponentialServer> r2{0.0};
}
```

The runner owns the `std::mt19937`, seeds it with the given value, and logs the seed
so every run can be reproduced.  Construct `ExponentialServer` with parameters by
specialising the template or wrapping it:

```cpp
template <typename TIME>
struct FastServer : public ExponentialServer<TIME> {
    FastServer() : ExponentialServer<TIME>(2.0) {}  // mu=2.0
};

cadmium::engine::stdevs::runner<double, FastServer> r{0.0, 42u};
```

---

## Implementing a coupled model

STDEVS coupled models share the same structure as classic DEVS coupled models
(CKW10 Section 4.3: closure under coupling is proven).  Use
`cadmium::modeling::devs::coupling` with a SELECT function.

```cpp
#include <cadmium/engine/devs_engine_helpers.hpp>   // first_imminent
#include <cadmium/modeling/coupling.hpp>
#include <cadmium/modeling/ports.hpp>

// Coupled-model output port
struct NetworkOut : public cadmium::out_port<Job> {};

// Submodels
template <typename TIME> struct Server1 : public ExponentialServer<TIME> {
    Server1() : ExponentialServer<TIME>(2.0) {}  // mu=2.0
};
template <typename TIME> struct Server2 : public ExponentialServer<TIME> {
    Server2() : ExponentialServer<TIME>(3.0) {}  // mu=3.0
};

using submodels = cadmium::modeling::models_tuple<Server1, Server2>;

// Route Server1 output → Server2 input; Server2 output → coupled output
using eics = std::tuple<>;
using eocs = std::tuple<
    cadmium::modeling::EOC<Server2, OutPort, NetworkOut>
>;
using ics = std::tuple<
    cadmium::modeling::IC<Server1, OutPort, Server2, InPort>
>;

template <typename TIME>
using TandemNetwork = cadmium::modeling::devs::coupling<
    TIME,
    std::tuple<>,              // input_ports
    std::tuple<NetworkOut>,    // output_ports
    submodels,
    eics, eocs, ics,
    cadmium::engine::devs::first_imminent   // SELECT
>;
```

### Coupling entry types

| Type | Connects |
|---|---|
| `EIC<ExternalPort, Submodel, SubmodelPort>` | coupled input → submodel input |
| `EOC<Submodel, SubmodelPort, ExternalPort>` | submodel output → coupled output |
| `IC<FromModel, FromPort, ToModel, ToPort>` | submodel output → submodel input |

### SELECT

`cadmium::engine::devs::first_imminent` (defined in
`<cadmium/engine/devs_engine_helpers.hpp>`) picks the submodel with the lowest
index in the models tuple when multiple are imminent at the same time.  To define
a custom priority, provide a struct with:

```cpp
struct MySelect {
    template <typename Engines, typename TIME>
    static std::size_t apply(const Engines& engines, TIME t_n);
};
```

---

## Running a coupled model

```cpp
cadmium::engine::stdevs::runner<double, TandemNetwork> r{0.0, 42u};
r.run_until(1000.0);
```

---

## DEVS vs PDEVS vs STDEVS — choosing a formalism

| Situation | Use |
|---|---|
| All transitions are deterministic | DEVS or PDEVS |
| Multiple models fire simultaneously; no explicit priority needed | PDEVS |
| Simultaneous events must be serialised (SELECT) | DEVS |
| Any transition draws from a probability distribution | STDEVS |
| QSS continuous integration (quantised ODE) | DEVS (QSS atomics are deterministic) |

STDEVS requires classic DEVS coupling (SELECT) because the coupling closure proof in
CKW10 relies on the classical DEVS coordinator, not the parallel one.  Do not mix
STDEVS atomics into a PDEVS coupled model.

---

## Available basic models

| Header | Model | Behaviour |
|---|---|---|
| `<cadmium/basic_model/stdevs/bernoulli_generator.hpp>` | `bernoulli_generator<MSG, TIME, URNG>` | CRTP; alternates between two periods with probability p; override `period_a()`, `period_b()`, `p()`, `output_message()` |
| `<cadmium/basic_model/stdevs/bernoulli_processor.hpp>` | `bernoulli_processor<MSG, TIME, URNG>` | CRTP; accepts arrivals with probability p_accept; override `processing_time()`, `p_accept()` |

---

## Further reading

- `example/stdevs/main-bernoulli-generator.cpp` — atomic STDEVS model; exact-replay
  demonstration
- `example/stdevs/main-stochastic-channel.cpp` — coupled STDEVS model (source +
  lossy channel)
- [cadmium-experiments](https://github.com/sdavtaker/cadmium-experiments) — CKW10 Load
  Balancer (LBM) and Networked Control System (NCS) experiments using STDEVS atomics
  with a manual event loop for mixed DEVS+STDEVS models
