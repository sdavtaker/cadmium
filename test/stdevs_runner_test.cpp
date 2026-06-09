// SPDX-License-Identifier: BSD-2-Clause
/**
 * Copyright (c) 2026-present, Damian Vicino
 * Carleton University
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <cadmium/engine/stdevs_runner.hpp>
#include <cadmium/modeling/coupling.hpp>
#include <cadmium/modeling/message_box.hpp>
#include <cadmium/modeling/ports.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <limits>
#include <random>

// ── Test models ────────────────────────────────────────────────────────────────
//
// exp_gen: each internal_transition draws a new period from Exp(1.0).
//   First event is deterministic at t = 1.0 (initial state = 1).
//
// flip_gen: Bernoulli(0.5) choice between period 1 and 2 (deterministic first
//   period = 1); used for coupled-model test.
//
// stdevs_counter: passive accumulator with immediate output on external input.

namespace {

    using RNG = std::mt19937;

    template <typename TIME> struct exp_gen {
        struct out : public cadmium::out_port<float> {};
        using state_type   = TIME;
        state_type state   = TIME{1};
        using input_ports  = std::tuple<>;
        using output_ports = std::tuple<out>;

        void internal_transition(RNG &rng) {
            std::exponential_distribution<double> dist(1.0);
            state = static_cast<TIME>(dist(rng));
        }

        void external_transition(TIME, typename cadmium::make_message_box<input_ports>::type,
                                 RNG &) {}

        typename cadmium::make_message_box<output_ports>::type output() const {
            typename cadmium::make_message_box<output_ports>::type box;
            cadmium::get_message<out>(box) = static_cast<float>(state);
            return box;
        }

        TIME time_advance() const {
            return state;
        }
    };

    template <typename TIME> struct flip_gen {
        struct out : public cadmium::out_port<int> {};
        using state_type   = TIME;
        state_type state   = TIME{1};
        using input_ports  = std::tuple<>;
        using output_ports = std::tuple<out>;

        void internal_transition(RNG &rng) {
            std::bernoulli_distribution coin(0.5);
            state = coin(rng) ? TIME{1} : TIME{2};
        }

        void external_transition(TIME, typename cadmium::make_message_box<input_ports>::type,
                                 RNG &) {}

        typename cadmium::make_message_box<output_ports>::type output() const {
            typename cadmium::make_message_box<output_ports>::type box;
            cadmium::get_message<out>(box) = 1;
            return box;
        }

        TIME time_advance() const {
            return state;
        }
    };

    template <typename TIME> struct stdevs_counter {
        struct in : public cadmium::in_port<int> {};
        struct count_out : public cadmium::out_port<int> {};

        struct state_type {
            int count       = 0;
            bool output_rdy = false;
        };
        state_type state{};

        using input_ports  = std::tuple<in>;
        using output_ports = std::tuple<count_out>;

        void internal_transition(RNG &) {
            state.output_rdy = false;
        }

        void external_transition(TIME, typename cadmium::make_message_box<input_ports>::type box,
                                 RNG &) {
            if (cadmium::get_message<in>(box).has_value()) {
                ++state.count;
                state.output_rdy = true;
            }
        }

        typename cadmium::make_message_box<output_ports>::type output() const {
            typename cadmium::make_message_box<output_ports>::type box;
            if (state.output_rdy)
                cadmium::get_message<count_out>(box) = state.count;
            return box;
        }

        TIME time_advance() const {
            return state.output_rdy ? TIME{0} : std::numeric_limits<TIME>::infinity();
        }
    };

} // anonymous namespace

// ── Coupled model for runner test ─────────────────────────────────────────────

struct runner_count_out : public cadmium::out_port<int> {};

using runner_submodels = cadmium::modeling::models_tuple<flip_gen, stdevs_counter>;
using runner_eics      = std::tuple<>;
using runner_eocs      = std::tuple<
         cadmium::modeling::EOC<stdevs_counter, stdevs_counter<float>::count_out, runner_count_out>>;
using runner_ics = std::tuple<cadmium::modeling::IC<flip_gen, flip_gen<float>::out, stdevs_counter,
                                                    stdevs_counter<float>::in>>;

template <typename TIME>
using flip_count_coupled =
    cadmium::modeling::devs::coupling<TIME, std::tuple<>, std::tuple<runner_count_out>,
                                      runner_submodels, runner_eics, runner_eocs, runner_ics,
                                      cadmium::engine::devs::first_imminent>;

// ── Runner type aliases ───────────────────────────────────────────────────────

using atomic_runner_t  = cadmium::engine::stdevs::runner<float, exp_gen>;
using coupled_runner_t = cadmium::engine::stdevs::runner<float, flip_count_coupled>;

// ── Atomic runner tests ───────────────────────────────────────────────────────

SCENARIO("stdevs atomic runner first next is deterministic regardless of seed",
         "[stdevs][runner][atomic]") {
    GIVEN("an atomic exp_gen runner (initial period = 1) seeded with 42") {
        atomic_runner_t r(0.0f, 42u);
        WHEN("run_until(0.5) is called (before first event at t=1)") {
            float t_end = r.run_until(0.5f);
            THEN("runner stops at t=1 (first scheduled event)") {
                CHECK(t_end == 1.0f);
            }
        }
    }
}

SCENARIO("stdevs atomic runner run_until advances past end time", "[stdevs][runner][atomic]") {
    GIVEN("an atomic exp_gen runner seeded with 1") {
        atomic_runner_t r(0.0f, 1u);
        WHEN("run_until(10.0) is called") {
            float t_end = r.run_until(10.0f);
            THEN("the returned time is >= 10.0") {
                CHECK(t_end >= 10.0f);
            }
        }
    }
}

SCENARIO("stdevs atomic runner exact replay: same seed produces same result",
         "[stdevs][runner][atomic][determinism]") {
    GIVEN("two exp_gen runners seeded with 77") {
        atomic_runner_t r1(0.0f, 77u);
        atomic_runner_t r2(0.0f, 77u);
        WHEN("both are run to t=50") {
            float t1 = r1.run_until(50.0f);
            float t2 = r2.run_until(50.0f);
            THEN("both stop at exactly the same time") {
                CHECK(t1 == t2);
            }
        }
    }
}

// ── Coupled runner tests ──────────────────────────────────────────────────────

SCENARIO("stdevs coupled runner initialises without error", "[stdevs][runner][coupled]") {
    GIVEN("a flip_count coupled runner seeded with 5") {
        coupled_runner_t r(0.0f, 5u);
        WHEN("run_until(3.0) is called") {
            float t_end = r.run_until(3.0f);
            THEN("runner advances past t=3") {
                CHECK(t_end >= 3.0f);
            }
        }
    }
}

SCENARIO("stdevs coupled runner exact replay with coupled model",
         "[stdevs][runner][coupled][determinism]") {
    GIVEN("two flip_count coupled runners seeded identically with 999") {
        coupled_runner_t r1(0.0f, 999u);
        coupled_runner_t r2(0.0f, 999u);
        WHEN("both are run to t=20") {
            float t1 = r1.run_until(20.0f);
            float t2 = r2.run_until(20.0f);
            THEN("both return the same stop time") {
                CHECK(t1 == t2);
            }
        }
    }
}

// ── Statistical test: exponential mean ───────────────────────────────────────

SCENARIO("stdevs exp_gen mean inter-arrival time is within 3 sigma of 1.0",
         "[stdevs][runner][statistical]") {
    GIVEN("an exp_gen runner seeded with 2025") {
        // Run until t = 1000; first event is at t=1 (initial state=1).
        // Subsequent events are exponential(1.0) → expect ~999 events after t=1.
        // Total events ≈ 1000, each with mean interval 1.0.
        // 3-sigma bound on mean: 3/sqrt(1000) ≈ 0.095.
        atomic_runner_t r(0.0f, 2025u);
        float t_end = r.run_until(1000.0f);
        WHEN("the last event time is inspected") {
            THEN("it is within 1.5 of 1000 (generous tolerance for the one partial interval)") {
                CHECK(t_end >= 998.5f);
                CHECK(t_end <= 1001.5f);
            }
        }
    }
}
