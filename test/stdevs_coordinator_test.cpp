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

#include <cadmium/engine/stdevs_coordinator.hpp>
#include <cadmium/engine/stdevs_engine_helpers.hpp>
#include <cadmium/engine/stdevs_simulator.hpp>
#include <cadmium/modeling/coupling.hpp>
#include <cadmium/modeling/message_box.hpp>
#include <cadmium/modeling/ports.hpp>

#include <catch2/catch_test_macros.hpp>
#include <limits>
#include <random>

// ── Test models ────────────────────────────────────────────────────────────────
//
// flip_gen: each internal_transition draws period ∈ {1, 2} via Bernoulli(0.5).
// stdevs_counter: passive model that receives int messages, increments a counter,
//   and immediately outputs the new count via a time-advance of 0.

namespace {

    using RNG = std::mt19937;

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

// ── Coupled model: flip_gen → stdevs_counter --EOC--> coupled_count_out ──────

struct coupled_count_out : public cadmium::out_port<int> {};

using gen_to_counter_submodels = cadmium::modeling::models_tuple<flip_gen, stdevs_counter>;
using gen_to_counter_eics      = std::tuple<>;
using gen_to_counter_eocs      = std::tuple<
         cadmium::modeling::EOC<stdevs_counter, stdevs_counter<float>::count_out, coupled_count_out>>;
using gen_to_counter_ics =
    std::tuple<cadmium::modeling::IC<flip_gen, flip_gen<float>::out, stdevs_counter,
                                     stdevs_counter<float>::in>>;

template <typename TIME>
using gen_to_counter_coupled =
    cadmium::modeling::devs::coupling<TIME, std::tuple<>, std::tuple<coupled_count_out>,
                                      gen_to_counter_submodels, gen_to_counter_eics,
                                      gen_to_counter_eocs, gen_to_counter_ics,
                                      cadmium::engine::devs::first_imminent>;

using coor_t = cadmium::engine::stdevs::coordinator<gen_to_counter_coupled, float, RNG>;

// ── Coordinator tests ─────────────────────────────────────────────────────────

SCENARIO("stdevs coordinator initialises with next = initial generator period",
         "[stdevs][coordinator][gen-to-counter]") {
    GIVEN("a gen_to_counter coordinator initialised at t=0") {
        RNG rng(42u);
        coor_t c;
        c.init(0.0f, rng);
        WHEN("next is queried") {
            THEN("it equals 1.0 (flip_gen initial period)") {
                CHECK(c.next() == 1.0f);
            }
        }
    }
}

SCENARIO("stdevs coordinator produces no output when generator fires (no EOC on gen)",
         "[stdevs][coordinator][gen-to-counter]") {
    GIVEN("a gen_to_counter coordinator initialised at t=0") {
        RNG rng(42u);
        coor_t c;
        c.init(0.0f, rng);
        WHEN("collect_outputs is called at t=1 (generator fires)") {
            c.collect_outputs(1.0f);
            THEN("the coupled output is empty — generator has no EOC") {
                CHECK(cadmium::engine::devs::all_box_empty(c.outbox()));
            }
        }
    }
}

SCENARIO("stdevs coordinator routes IC and counter outputs on EOC after one generator tick",
         "[stdevs][coordinator][gen-to-counter]") {
    GIVEN("a gen_to_counter coordinator initialised at t=0") {
        RNG rng(42u);
        coor_t c;
        c.init(0.0f, rng);

        // Step 1 — generator fires at t=1: no coupled output
        c.collect_outputs(1.0f);
        REQUIRE(cadmium::engine::devs::all_box_empty(c.outbox()));
        c.advance_simulation(1.0f);
        // counter received a message → state.output_rdy=true → ta=0 → next=1
        REQUIRE(c.next() == 1.0f);

        // Step 2 — counter fires its internal transition (ta=0) at t=1
        WHEN("collect_outputs and advance are called for the counter's internal event") {
            c.collect_outputs(1.0f);
            THEN("the coupled output port holds count = 1") {
                const auto &opt = cadmium::get_message<coupled_count_out>(c.outbox());
                REQUIRE(opt.has_value());
                CHECK(opt.value() == 1);
            }
            c.advance_simulation(1.0f);
        }
    }
}

SCENARIO("stdevs coordinator accumulates count correctly over three ticks",
         "[stdevs][coordinator][gen-to-counter]") {
    GIVEN("a gen_to_counter coordinator initialised at t=0") {
        RNG rng(7u);
        coor_t c;
        c.init(0.0f, rng);

        int last_count = 0;
        // Drive 3 full generator→counter cycles; at each cycle the counter must
        // output a value one greater than the last.
        for (int tick = 1; tick <= 3; ++tick) {
            // Generator fires
            float t_gen = c.next();
            c.collect_outputs(t_gen);
            c.advance_simulation(t_gen);
            // Counter fires (ta=0 at t_gen)
            REQUIRE(c.next() == t_gen);
            c.collect_outputs(t_gen);
            const auto &opt = cadmium::get_message<coupled_count_out>(c.outbox());
            REQUIRE(opt.has_value());
            CHECK(opt.value() == tick);
            last_count = opt.value();
            c.advance_simulation(t_gen);
        }
        REQUIRE(last_count == 3);
    }
}

SCENARIO("stdevs coordinator rejects collect_outputs past next event",
         "[stdevs][coordinator][error]") {
    GIVEN("a gen_to_counter coordinator with next = 1.0") {
        RNG rng(1u);
        coor_t c;
        c.init(0.0f, rng);
        WHEN("collect_outputs is called at t=5 (past next)") {
            THEN("a domain_error is thrown") {
                CHECK_THROWS_AS(c.collect_outputs(5.0f), std::domain_error);
            }
        }
    }
}

SCENARIO("stdevs coordinator rejects advance_simulation outside time scope",
         "[stdevs][coordinator][error]") {
    GIVEN("a gen_to_counter coordinator with next = 1.0") {
        RNG rng(1u);
        coor_t c;
        c.init(0.0f, rng);
        c.collect_outputs(1.0f);
        WHEN("advance_simulation is called at t=5 (past next)") {
            THEN("a domain_error is thrown") {
                CHECK_THROWS_AS(c.advance_simulation(5.0f), std::domain_error);
            }
        }
    }
}
