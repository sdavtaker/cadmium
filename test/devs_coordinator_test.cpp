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

#include <cadmium/basic_model/devs/accumulator.hpp>
#include <cadmium/basic_model/devs/generator.hpp>
#include <cadmium/engine/devs_coordinator.hpp>
#include <cadmium/engine/devs_engine_helpers.hpp>
#include <cadmium/engine/devs_simulator.hpp>
#include <cadmium/modeling/coupling.hpp>
#include <cadmium/modeling/message_box.hpp>

#include <catch2/catch_test_macros.hpp>
#include <limits>
#include <ostream>

// ── Shared types ───────────────────────────────────────────────────────────

struct tick {
    friend std::ostream &operator<<(std::ostream &os, const tick &) {
        return os << "tick{}";
    }
};

template <typename TIME>
struct tick_generator
    : public cadmium::basic_models::devs::generator<tick_generator<TIME>, tick, TIME> {
    float period() const {
        return 1.0f;
    }
    tick output_message() const {
        return tick{};
    }
};

using tick_gen_defs = cadmium::basic_models::devs::generator_defs<tick>;

// ── Single-generator coupled model ────────────────────────────────────────
//
//   [tick_generator] --EOC--> (coupled_tick_out)
//
//   SELECT: first_imminent (only one submodel, so trivial)

struct coupled_tick_out : public cadmium::out_port<tick> {};

using gen_oports    = std::tuple<coupled_tick_out>;
using gen_submodels = cadmium::modeling::models_tuple<tick_generator>;
using gen_eics      = std::tuple<>;
using gen_eocs =
    std::tuple<cadmium::modeling::EOC<tick_generator, tick_gen_defs::out, coupled_tick_out>>;
using gen_ics = std::tuple<>;

template <typename TIME>
using single_gen_coupled =
    cadmium::modeling::devs::coupling<TIME, std::tuple<>, gen_oports, gen_submodels, gen_eics,
                                      gen_eocs, gen_ics, cadmium::engine::devs::first_imminent>;

using gen_coor_t = cadmium::engine::devs::coordinator<single_gen_coupled, float>;

SCENARIO("devs coordinator wrapping a single generator initialises at next = period",
         "[devs][coordinator][generator]") {
    GIVEN("a coordinator around a tick_generator with period 1") {
        gen_coor_t c;
        c.init(0.0f);
        WHEN("next is queried") {
            THEN("it equals 1.0 (one period)") {
                CHECK(c.next() == 1.0f);
            }
        }
    }
}

SCENARIO("devs coordinator collects generator output at scheduled time",
         "[devs][coordinator][generator]") {
    GIVEN("a coordinator initialised at t=0") {
        gen_coor_t c;
        c.init(0.0f);
        WHEN("collect_outputs is called at t=1") {
            c.collect_outputs(1.0f);
            THEN("the coupled output port holds the tick") {
                const auto &opt = cadmium::get_message<coupled_tick_out>(c.outbox());
                CHECK(opt.has_value());
            }
        }
    }
}

SCENARIO("devs coordinator produces no output before the scheduled time",
         "[devs][coordinator][generator]") {
    GIVEN("a coordinator initialised at t=0") {
        gen_coor_t c;
        c.init(0.0f);
        WHEN("collect_outputs is called at t=0.5 (before next)") {
            c.collect_outputs(0.5f);
            THEN("the outbox is empty") {
                CHECK(cadmium::engine::devs::all_box_empty(c.outbox()));
            }
        }
    }
}

SCENARIO("devs coordinator advance reschedules the generator", "[devs][coordinator][generator]") {
    GIVEN("a coordinator initialised at t=0") {
        gen_coor_t c;
        c.init(0.0f);
        WHEN("collect_outputs(1) then advance_simulation(1) are called") {
            c.collect_outputs(1.0f);
            c.advance_simulation(1.0f);
            THEN("next is now 2.0") {
                CHECK(c.next() == 2.0f);
            }
        }
    }
}

SCENARIO("devs coordinator rejects collecting past the next event",
         "[devs][coordinator][generator]") {
    GIVEN("a coordinator initialised at t=0 (next=1)") {
        gen_coor_t c;
        c.init(0.0f);
        WHEN("collect_outputs is called at t=2 (past next)") {
            THEN("a domain_error is thrown") {
                CHECK_THROWS_AS(c.collect_outputs(2.0f), std::domain_error);
            }
        }
    }
}

// ── Generator-to-accumulator coupled model ────────────────────────────────
//
//   [int_gen] --IC--> [accumulator]
//   [reset_gen] --IC--> [accumulator]
//   [accumulator] --EOC--> (coupled_sum_out)
//
//   Models tuple order: int_gen (0), reset_gen (1), accumulator (2)
//   SELECT: first_imminent — int_gen has priority at t=5 when both int_gen and
//   reset_gen are imminent.

template <typename TIME>
struct int_gen : public cadmium::basic_models::devs::generator<int_gen<TIME>, int, TIME> {
    TIME period() const {
        return TIME{1};
    }
    int output_message() const {
        return 1;
    }
};

template <typename TIME>
struct reset_gen
    : public cadmium::basic_models::devs::generator<
          reset_gen<TIME>, cadmium::basic_models::devs::accumulator_defs<int>::reset_tick, TIME> {
    TIME period() const {
        return TIME{5};
    }
    cadmium::basic_models::devs::accumulator_defs<int>::reset_tick output_message() const {
        return {};
    }
};

template <typename TIME> using int_acc = cadmium::basic_models::devs::accumulator<int, TIME>;
using acc_defs                         = cadmium::basic_models::devs::accumulator_defs<int>;

struct g2a_sum_out : public cadmium::out_port<int> {};

using g2a_submodels = cadmium::modeling::models_tuple<int_gen, reset_gen, int_acc>;
using g2a_eics      = std::tuple<>;
using g2a_eocs      = std::tuple<cadmium::modeling::EOC<int_acc, acc_defs::sum, g2a_sum_out>>;
using g2a_ics       = std::tuple<
          cadmium::modeling::IC<int_gen, cadmium::basic_models::devs::generator_defs<int>::out, int_acc,
                                acc_defs::add>,
          cadmium::modeling::IC<reset_gen,
                                cadmium::basic_models::devs::generator_defs<acc_defs::reset_tick>::out,
                                int_acc, acc_defs::reset>>;

template <typename TIME>
using g2a_coupled = cadmium::modeling::devs::coupling<TIME, std::tuple<>, std::tuple<g2a_sum_out>,
                                                      g2a_submodels, g2a_eics, g2a_eocs, g2a_ics,
                                                      cadmium::engine::devs::first_imminent>;

using g2a_coor_t = cadmium::engine::devs::coordinator<g2a_coupled, float>;

SCENARIO("devs gen-to-acc coordinator initialises with next at 1 (int_gen period)",
         "[devs][coordinator][count-fives]") {
    GIVEN("a gen-to-acc coordinator initialised at t=0") {
        g2a_coor_t c;
        c.init(0.0f);
        WHEN("next is queried") {
            THEN("it is 1.0 (int_gen fires first)") {
                CHECK(c.next() == 1.0f);
            }
        }
    }
}

SCENARIO("devs gen-to-acc coordinator SELECT picks int_gen at t=1 — no EOC output",
         "[devs][coordinator][count-fives]") {
    GIVEN("a gen-to-acc coordinator initialised at t=0") {
        g2a_coor_t c;
        c.init(0.0f);
        WHEN("collect_outputs and advance are called at t=1") {
            c.collect_outputs(1.0f);
            THEN("outbox is empty (int_gen has no EOC)") {
                CHECK(cadmium::engine::devs::all_box_empty(c.outbox()));
            }
            c.advance_simulation(1.0f);
            AND_THEN("next advances to t=2") {
                CHECK(c.next() == 2.0f);
            }
        }
    }
}

// Helper: advance the coordinator through n ticks of size dt from t_start
static void advance_n(g2a_coor_t &c, float t_start, int n, float dt) {
    for (int i = 0; i < n; ++i) {
        float t = t_start + (i + 1) * dt;
        c.collect_outputs(t);
        c.advance_simulation(t);
    }
}

SCENARIO("devs gen-to-acc coordinator accumulates 5 and outputs on reset",
         "[devs][coordinator][count-fives]") {
    GIVEN("a coordinator running from t=0 to t=5, advancing one model per tick") {
        g2a_coor_t c;
        c.init(0.0f);

        // Advance int_gen ticks at t=1,2,3,4 (accumulator state → 4)
        advance_n(c, 0.0f, 4, 1.0f);

        // At t=5: int_gen is imminent (t_n=5) and reset_gen is also imminent (t_n=5).
        // SELECT (first_imminent) picks int_gen first.
        REQUIRE(c.next() == 5.0f);
        c.collect_outputs(5.0f);
        REQUIRE(cadmium::engine::devs::all_box_empty(c.outbox())); // int_gen has no EOC
        c.advance_simulation(5.0f);
        // Now int_gen fired (acc state → 5, on_reset=false).
        // reset_gen is still at t_n=5; coordinator next should still be 5.
        REQUIRE(c.next() == 5.0f);

        // Second advance at t=5: reset_gen fires → acc receives reset → acc enters
        // on_reset state with time_advance()=0 → acc t_n=5.
        c.collect_outputs(5.0f);
        REQUIRE(cadmium::engine::devs::all_box_empty(c.outbox())); // reset_gen has no EOC
        c.advance_simulation(5.0f);
        // Accumulator now has on_reset=true, time_advance()=0, so t_n(acc)=5.
        REQUIRE(c.next() == 5.0f);

        // Third advance at t=5: accumulator fires internal_transition → outputs sum=5.
        WHEN("collect_outputs is called for the accumulator's internal event") {
            c.collect_outputs(5.0f);
            THEN("the coupled sum output port holds 5") {
                const auto &opt = cadmium::get_message<g2a_sum_out>(c.outbox());
                REQUIRE(opt.has_value());
                CHECK(opt.value() == 5);
            }
            c.advance_simulation(5.0f);
            AND_THEN("after the reset drains, next jumps to 6 (int_gen)") {
                CHECK(c.next() == 6.0f);
            }
        }
    }
}
