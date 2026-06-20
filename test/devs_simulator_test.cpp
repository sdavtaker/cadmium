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
#include <cadmium/engine/devs_simulator.hpp>

#include <catch2/catch_test_macros.hpp>
#include <limits>

// ── Accumulator simulator ──────────────────────────────────────────────────

template <typename TIME>
using int_accumulator      = cadmium::basic_models::devs::accumulator<int, TIME>;
using int_accumulator_defs = cadmium::basic_models::devs::accumulator_defs<int>;

using sim_t     = cadmium::engine::devs::simulator<int_accumulator, float>;
using in_box_t  = typename cadmium::make_message_box<int_accumulator<float>::input_ports>::type;
using out_box_t = typename cadmium::make_message_box<int_accumulator<float>::output_ports>::type;

SCENARIO("devs accumulator simulator starts in passive state", "[devs][simulator][accumulator]") {
    GIVEN("a freshly initialised accumulator simulator") {
        sim_t s;
        s.init(0.0f);
        WHEN("next scheduled time is queried") {
            THEN("it is infinity") {
                CHECK(s.next() == std::numeric_limits<float>::infinity());
            }
        }
    }
}

SCENARIO("devs accumulator simulator receiving only an add message stays passive",
         "[devs][simulator][accumulator]") {
    GIVEN("a freshly initialised accumulator simulator") {
        sim_t s;
        s.init(0.0f);
        WHEN("an add message is placed in the inbox and simulation is advanced") {
            in_box_t box{};
            cadmium::get_message<int_accumulator_defs::add>(box) = 7;
            s.inbox(box);
            s.advance_simulation(3.0f);
            THEN("next remains infinity because no reset was received") {
                CHECK(s.next() == std::numeric_limits<float>::infinity());
            }
        }
    }
}

SCENARIO("devs accumulator simulator schedules internal transition when reset is received",
         "[devs][simulator][accumulator]") {
    GIVEN("an accumulator that received an add then a reset") {
        sim_t s;
        s.init(0.0f);
        in_box_t add_box{};
        cadmium::get_message<int_accumulator_defs::add>(add_box) = 5;
        s.inbox(add_box);
        s.advance_simulation(2.0f);
        WHEN("a reset message arrives at time 4") {
            in_box_t reset_box{};
            cadmium::get_message<int_accumulator_defs::reset>(reset_box) =
                int_accumulator_defs::reset_tick{};
            s.inbox(reset_box);
            s.advance_simulation(4.0f);
            THEN("next is scheduled at time 4 (time_advance returns 0 on reset state)") {
                CHECK(s.next() == 4.0f);
            }
        }
    }
}

SCENARIO("devs accumulator simulator output is the accumulated value",
         "[devs][simulator][accumulator]") {
    GIVEN("an accumulator that has received an add of 5 and is then reset") {
        sim_t s;
        s.init(0.0f);
        in_box_t add_box{};
        cadmium::get_message<int_accumulator_defs::add>(add_box) = 5;
        s.inbox(add_box);
        s.advance_simulation(2.0f);
        in_box_t reset_box{};
        cadmium::get_message<int_accumulator_defs::reset>(reset_box) =
            int_accumulator_defs::reset_tick{};
        s.inbox(reset_box);
        s.advance_simulation(4.0f);
        WHEN("outputs are collected at the scheduled time") {
            s.collect_outputs(4.0f);
            const auto &o = s.outbox();
            THEN("the sum port holds exactly 5") {
                const auto &sum_opt = cadmium::get_message<int_accumulator_defs::sum>(o);
                REQUIRE(sum_opt.has_value());
                CHECK(sum_opt.value() == 5);
            }
        }
    }
}

SCENARIO("devs accumulator simulator internal transition clears the counter",
         "[devs][simulator][accumulator]") {
    GIVEN("an accumulator that was reset once and the internal transition drained") {
        sim_t s;
        s.init(0.0f);
        in_box_t reset_box{};
        cadmium::get_message<int_accumulator_defs::reset>(reset_box) =
            int_accumulator_defs::reset_tick{};
        s.inbox(reset_box);
        s.advance_simulation(3.0f);
        s.advance_simulation(3.0f); // internal transition fires (empty inbox, t==next)
        REQUIRE(s.next() == std::numeric_limits<float>::infinity());
        WHEN("a second reset arrives at time 5 with no prior add") {
            in_box_t reset_box2{};
            cadmium::get_message<int_accumulator_defs::reset>(reset_box2) =
                int_accumulator_defs::reset_tick{};
            s.inbox(reset_box2);
            s.advance_simulation(5.0f);
            s.collect_outputs(5.0f);
            const auto &o   = s.outbox();
            const auto &opt = cadmium::get_message<int_accumulator_defs::sum>(o);
            THEN("the output sum is 0") {
                REQUIRE(opt.has_value());
                CHECK(opt.value() == 0);
            }
        }
    }
}

SCENARIO("devs accumulator simulator outbox is empty before scheduled time",
         "[devs][simulator][accumulator]") {
    GIVEN("an accumulator in reset state with next at time 4") {
        sim_t s;
        s.init(0.0f);
        in_box_t reset_box{};
        cadmium::get_message<int_accumulator_defs::reset>(reset_box) =
            int_accumulator_defs::reset_tick{};
        s.inbox(reset_box);
        s.advance_simulation(4.0f);
        WHEN("outputs are collected at time 2 (before next)") {
            s.collect_outputs(2.0f);
            THEN("all box entries are empty") {
                CHECK(cadmium::engine::devs::all_box_empty(s.outbox()));
            }
        }
    }
}

SCENARIO("devs accumulator simulator rejects advancing to a time in the past",
         "[devs][simulator][accumulator]") {
    GIVEN("an accumulator advanced to time 3") {
        sim_t s;
        s.init(0.0f);
        in_box_t reset_box{};
        cadmium::get_message<int_accumulator_defs::reset>(reset_box) =
            int_accumulator_defs::reset_tick{};
        s.inbox(reset_box);
        s.advance_simulation(3.0f);
        WHEN("advance_simulation is called with time 2 (in the past)") {
            in_box_t empty{};
            s.inbox(empty);
            THEN("a domain_error is thrown") {
                CHECK_THROWS_AS(s.advance_simulation(2.0f), std::domain_error);
            }
        }
    }
}

SCENARIO("devs accumulator simulator rejects advancing past a scheduled internal event",
         "[devs][simulator][accumulator]") {
    GIVEN("an accumulator with an internal event at time 3") {
        sim_t s;
        s.init(0.0f);
        in_box_t reset_box{};
        cadmium::get_message<int_accumulator_defs::reset>(reset_box) =
            int_accumulator_defs::reset_tick{};
        s.inbox(reset_box);
        s.advance_simulation(3.0f);
        WHEN("advance_simulation is called at time 4 (past the scheduled event)") {
            in_box_t empty{};
            s.inbox(empty);
            THEN("a domain_error is thrown") {
                CHECK_THROWS_AS(s.advance_simulation(4.0f), std::domain_error);
            }
        }
    }
}

// ── Generator simulator ────────────────────────────────────────────────────

const float gen_period  = 1.0f;
const float gen_message = 2.0f;

using float_generator_defs = cadmium::basic_models::devs::generator_defs<float>;

template <typename TIME>
struct float_generator
    : public cadmium::basic_models::devs::generator<float_generator<TIME>, float, TIME> {
    float period() const {
        return gen_period;
    }
    float output_message() const {
        return gen_message;
    }
};

using gen_sim_t = cadmium::engine::devs::simulator<float_generator, float>;

SCENARIO("devs generator simulator schedules first output one period after initialisation",
         "[devs][simulator][generator]") {
    GIVEN("a generator simulator initialised at time 0 with period 1") {
        gen_sim_t s;
        s.init(0.0f);
        WHEN("next is queried") {
            THEN("it equals the period") {
                CHECK(s.next() == 1.0f);
            }
        }
    }
}

SCENARIO("devs generator simulator produces no output before the scheduled time",
         "[devs][simulator][generator]") {
    GIVEN("a generator simulator initialised at time 0") {
        gen_sim_t s;
        s.init(0.0f);
        WHEN("outputs are collected at time 0.5 (before the first scheduled tick)") {
            s.collect_outputs(0.5f);
            THEN("all outbox entries are empty") {
                CHECK(cadmium::engine::devs::all_box_empty(s.outbox()));
            }
        }
    }
}

SCENARIO("devs generator simulator produces the configured output at the scheduled time",
         "[devs][simulator][generator]") {
    GIVEN("a generator simulator initialised at time 0") {
        gen_sim_t s;
        s.init(0.0f);
        WHEN("outputs are collected at the scheduled time of 1.0") {
            s.collect_outputs(1.0f);
            const auto &out = s.outbox();
            THEN("the out port holds exactly the configured message") {
                const auto &opt = cadmium::get_message<float_generator_defs::out>(out);
                REQUIRE(opt.has_value());
                CHECK(opt.value() == 2.0f);
            }
        }
    }
}

SCENARIO("devs generator simulator reschedules the next output after advancing simulation",
         "[devs][simulator][generator]") {
    GIVEN("a generator simulator initialised at time 0") {
        gen_sim_t s;
        s.init(0.0f);
        WHEN("simulation is advanced to time 1") {
            s.advance_simulation(1.0f);
            THEN("next is scheduled at time 2") {
                CHECK(s.next() == 2.0f);
            }
        }
    }
}
