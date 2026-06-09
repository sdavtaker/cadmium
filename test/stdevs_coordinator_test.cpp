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

#include <cadmium/basic_model/stdevs/bernoulli_generator.hpp>
#include <cadmium/basic_model/stdevs/bernoulli_processor.hpp>
#include <cadmium/engine/stdevs_coordinator.hpp>
#include <cadmium/engine/stdevs_engine_helpers.hpp>
#include <cadmium/modeling/coupling.hpp>
#include <cadmium/modeling/message_box.hpp>

#include <catch2/catch_test_macros.hpp>
#include <limits>
#include <random>

// ── Concrete submodels ────────────────────────────────────────────────────────
//
// int_flip_gen: Bernoulli(0.5) periods {1, 2}, outputs constant 1.
// half_unit_processor: always accepts (p_accept=1.0), processing_time=0.5.
//   Using 0.5 guarantees the processor fires before the next generator tick
//   (min generator period = 1 > 0.5), so coordinator next is deterministic.

namespace {

    using RNG = std::mt19937;

    template <typename TIME>
    struct int_flip_gen
        : public cadmium::basic_models::stdevs::bernoulli_generator<int, TIME, RNG> {
        TIME period_a() const override {
            return TIME{1};
        }
        TIME period_b() const override {
            return TIME{2};
        }
        double p() const override {
            return 0.5;
        }
        int output_message() const override {
            return 1;
        }
    };

    template <typename TIME>
    struct half_unit_processor
        : public cadmium::basic_models::stdevs::bernoulli_processor<int, TIME, RNG> {
        TIME processing_time() const override {
            return TIME{0.5f};
        }
        double p_accept() const override {
            return 1.0;
        }
    };

} // anonymous namespace

// ── Port aliases ──────────────────────────────────────────────────────────────

using gen_defs  = cadmium::basic_models::stdevs::bernoulli_generator_defs<int>;
using proc_defs = cadmium::basic_models::stdevs::bernoulli_processor_defs<int>;

// ── Coupled model: int_flip_gen --IC--> half_unit_processor --EOC--> out ──────
//
// Topology:
//   [int_flip_gen] --IC(gen::out → proc::in)--> [half_unit_processor]
//   [half_unit_processor] --EOC(proc::out → coupled_out)
//
// SELECT: first_imminent (gen is at index 0, so it has priority when tied).

struct coupled_out : public cadmium::out_port<int> {};

using gen_proc_submodels = cadmium::modeling::models_tuple<int_flip_gen, half_unit_processor>;
using gen_proc_eics      = std::tuple<>;
using gen_proc_eocs =
    std::tuple<cadmium::modeling::EOC<half_unit_processor, proc_defs::out, coupled_out>>;
using gen_proc_ics = std::tuple<
    cadmium::modeling::IC<int_flip_gen, gen_defs::out, half_unit_processor, proc_defs::in>>;

template <typename TIME>
using gen_proc_coupled =
    cadmium::modeling::devs::coupling<TIME, std::tuple<>, std::tuple<coupled_out>,
                                      gen_proc_submodels, gen_proc_eics, gen_proc_eocs,
                                      gen_proc_ics, cadmium::engine::devs::first_imminent>;

using coor_t = cadmium::engine::stdevs::coordinator<gen_proc_coupled, float, RNG>;

// ── Coordinator tests ─────────────────────────────────────────────────────────

SCENARIO("stdevs coordinator initialises with next = generator period_a",
         "[stdevs][coordinator][gen-proc]") {
    GIVEN("a gen_proc coordinator initialised at t=0") {
        RNG rng(42u);
        coor_t c;
        c.init(0.0f, rng);
        WHEN("next is queried") {
            THEN("it equals 1.0 (int_flip_gen initial period_a)") {
                CHECK(c.next() == 1.0f);
            }
        }
    }
}

SCENARIO("stdevs coordinator produces no coupled output when generator fires",
         "[stdevs][coordinator][gen-proc]") {
    GIVEN("a gen_proc coordinator initialised at t=0") {
        RNG rng(42u);
        coor_t c;
        c.init(0.0f, rng);
        WHEN("collect_outputs is called at t=1 (generator event)") {
            c.collect_outputs(1.0f);
            THEN("outbox is empty — generator has no EOC") {
                CHECK(cadmium::engine::devs::all_box_empty(c.outbox()));
            }
        }
    }
}

SCENARIO("stdevs coordinator routes IC: processor schedules after generator fires",
         "[stdevs][coordinator][gen-proc]") {
    GIVEN("a gen_proc coordinator at t=0, gen fires at t=1") {
        RNG rng(42u);
        coor_t c;
        c.init(0.0f, rng);
        WHEN("gen fires (collect + advance at t=1)") {
            c.collect_outputs(1.0f);
            c.advance_simulation(1.0f);
            THEN("next = 1.5 (processor fires at t=1 + processing_time=0.5)") {
                // gen min period=1, so gen.next ≥ 2; processor.next=1.5 → min=1.5
                CHECK(c.next() == 1.5f);
            }
        }
    }
}

SCENARIO("stdevs coordinator EOC: processor output reaches coupled output port",
         "[stdevs][coordinator][gen-proc]") {
    GIVEN("a gen_proc coordinator after gen fires at t=1 and processor fires at t=1.5") {
        RNG rng(42u);
        coor_t c;
        c.init(0.0f, rng);
        c.collect_outputs(1.0f);
        c.advance_simulation(1.0f);
        REQUIRE(c.next() == 1.5f);
        WHEN("collect_outputs and advance are called at t=1.5 (processor fires)") {
            c.collect_outputs(1.5f);
            THEN("the coupled output port holds the forwarded message = 1") {
                const auto &opt = cadmium::get_message<coupled_out>(c.outbox());
                REQUIRE(opt.has_value());
                CHECK(opt.value() == 1);
            }
            c.advance_simulation(1.5f);
            AND_THEN("processor returns to idle; next is the generator's next scheduled tick") {
                // processor.next = inf; gen.next is either 2 or 3
                float n = c.next();
                CHECK((n == 2.0f || n == 3.0f));
            }
        }
    }
}

SCENARIO("stdevs coordinator forwards three successive generator ticks correctly",
         "[stdevs][coordinator][gen-proc]") {
    GIVEN("a gen_proc coordinator driven for 3 full gen→proc cycles") {
        RNG rng(7u);
        coor_t c;
        c.init(0.0f, rng);
        for (int tick = 1; tick <= 3; ++tick) {
            float t_gen = c.next();
            c.collect_outputs(t_gen);
            c.advance_simulation(t_gen);
            float t_proc = c.next(); // should be t_gen + 0.5
            REQUIRE(t_proc == t_gen + 0.5f);
            c.collect_outputs(t_proc);
            const auto &opt = cadmium::get_message<coupled_out>(c.outbox());
            REQUIRE(opt.has_value());
            CHECK(opt.value() == 1);
            c.advance_simulation(t_proc);
        }
    }
}

// ── Error conditions ──────────────────────────────────────────────────────────

SCENARIO("stdevs coordinator rejects collect_outputs past next event",
         "[stdevs][coordinator][error]") {
    GIVEN("a gen_proc coordinator with next = 1.0") {
        RNG rng(1u);
        coor_t c;
        c.init(0.0f, rng);
        WHEN("collect_outputs is called at t=5") {
            THEN("a domain_error is thrown") {
                CHECK_THROWS_AS(c.collect_outputs(5.0f), std::domain_error);
            }
        }
    }
}

SCENARIO("stdevs coordinator rejects advance_simulation outside time scope",
         "[stdevs][coordinator][error]") {
    GIVEN("a gen_proc coordinator with next = 1.0") {
        RNG rng(1u);
        coor_t c;
        c.init(0.0f, rng);
        c.collect_outputs(1.0f);
        WHEN("advance_simulation is called at t=5") {
            THEN("a domain_error is thrown") {
                CHECK_THROWS_AS(c.advance_simulation(5.0f), std::domain_error);
            }
        }
    }
}
