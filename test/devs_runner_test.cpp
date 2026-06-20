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
#include <cadmium/engine/devs_runner.hpp>
#include <cadmium/modeling/coupling.hpp>
#include <cadmium/modeling/message_box.hpp>

#include <catch2/catch_test_macros.hpp>
#include <limits>
#include <ostream>

// ── Generator coupled model ────────────────────────────────────────────────

struct tick {
    friend std::ostream &operator<<(std::ostream &os, const tick &) {
        return os << "tick{}";
    }
};

template <typename TIME>
struct tick_generator
    : public cadmium::basic_models::devs::generator<tick_generator<TIME>, tick, TIME> {
    TIME period() const {
        return TIME{1};
    }
    tick output_message() const {
        return tick{};
    }
};

using tick_gen_defs = cadmium::basic_models::devs::generator_defs<tick>;
struct gen_coupled_out : public cadmium::out_port<tick> {};

template <typename TIME>
using gen_coupled = cadmium::modeling::devs::coupling<
    TIME, std::tuple<>, std::tuple<gen_coupled_out>,
    cadmium::modeling::models_tuple<tick_generator>, std::tuple<>,
    std::tuple<cadmium::modeling::EOC<tick_generator, tick_gen_defs::out, gen_coupled_out>>,
    std::tuple<>, cadmium::engine::devs::first_imminent>;

SCENARIO("devs runner with a single generator runs until the given time",
         "[devs][runner][generator]") {
    GIVEN("a runner around gen_coupled initialised at t=0") {
        cadmium::engine::devs::runner<float, gen_coupled> r(0.0f);
        WHEN("run_until(3.0) is called") {
            float t_end = r.run_until(3.0f);
            THEN("runner stops at t=3 (the first tick at or after 3)") {
                CHECK(t_end == 3.0f);
            }
        }
    }
}

SCENARIO("devs runner returns infinity when simulation passivates", "[devs][runner][generator]") {
    GIVEN("a runner around gen_coupled initialised at t=0") {
        cadmium::engine::devs::runner<float, gen_coupled> r(0.0f);
        WHEN("run_until(0.5) is called (before first tick)") {
            float t_end = r.run_until(0.5f);
            THEN("runner stops immediately since next > 0.5") {
                CHECK(t_end == 1.0f);
            }
        }
    }
}

// ── Count-fives runner test ────────────────────────────────────────────────
//
//  int_gen(period=1) --IC--> accumulator
//  reset_gen(period=5) --IC--> accumulator
//  accumulator --EOC--> count_fives_out
//
//  At each multiple of 5 the accumulator outputs 5 (SELECT: int_gen first).

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

struct count_fives_out : public cadmium::out_port<int> {};

template <typename TIME>
using count_fives_coupled = cadmium::modeling::devs::coupling<
    TIME, std::tuple<>, std::tuple<count_fives_out>,
    cadmium::modeling::models_tuple<int_gen, reset_gen, int_acc>, std::tuple<>,
    std::tuple<cadmium::modeling::EOC<int_acc, acc_defs::sum, count_fives_out>>,
    std::tuple<
        cadmium::modeling::IC<int_gen, cadmium::basic_models::devs::generator_defs<int>::out,
                              int_acc, acc_defs::add>,
        cadmium::modeling::IC<
            reset_gen, cadmium::basic_models::devs::generator_defs<acc_defs::reset_tick>::out,
            int_acc, acc_defs::reset>>,
    cadmium::engine::devs::first_imminent>;

SCENARIO("devs runner drives count-fives to t=10 without error", "[devs][runner][count-fives]") {
    GIVEN("a count-fives runner initialised at t=0") {
        cadmium::engine::devs::runner<float, count_fives_coupled> r(0.0f);
        WHEN("run_until(10.0) is called") {
            float t_end = r.run_until(10.0f);
            THEN("runner exits at or after t=10") {
                CHECK(t_end >= 10.0f);
            }
        }
    }
}

SCENARIO("devs runner next() advances to 6 after draining t=5 in count-fives",
         "[devs][runner][count-fives]") {
    GIVEN("a count-fives runner initialised at t=0") {
        cadmium::engine::devs::runner<float, count_fives_coupled> r(0.0f);
        WHEN("run_until(5.5) is called (stops after t=5 is fully drained)") {
            float t_end = r.run_until(5.5f);
            THEN("next event is at t=6 (int_gen fires next)") {
                CHECK(t_end == 6.0f);
            }
        }
    }
}
