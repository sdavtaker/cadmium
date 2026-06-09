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
#include <cadmium/engine/stdevs_runner.hpp>
#include <cadmium/modeling/coupling.hpp>
#include <cadmium/modeling/message_box.hpp>

#include <catch2/catch_test_macros.hpp>
#include <limits>
#include <random>

// ── Concrete submodels ────────────────────────────────────────────────────────

namespace {

    using RNG = std::mt19937;

    // int_flip_gen: Bernoulli(0.5), periods {1, 2}, first event at t=1.
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

    // half_unit_processor: always accepts, processing_time = 0.5.
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

struct runner_out : public cadmium::out_port<int> {};

using runner_submodels = cadmium::modeling::models_tuple<int_flip_gen, half_unit_processor>;
using runner_eics      = std::tuple<>;
using runner_eocs =
    std::tuple<cadmium::modeling::EOC<half_unit_processor, proc_defs::out, runner_out>>;
using runner_ics = std::tuple<
    cadmium::modeling::IC<int_flip_gen, gen_defs::out, half_unit_processor, proc_defs::in>>;

template <typename TIME>
using gen_proc_coupled =
    cadmium::modeling::devs::coupling<TIME, std::tuple<>, std::tuple<runner_out>, runner_submodels,
                                      runner_eics, runner_eocs, runner_ics,
                                      cadmium::engine::devs::first_imminent>;

// ── Runner type aliases ───────────────────────────────────────────────────────

using atomic_runner_t  = cadmium::engine::stdevs::runner<float, int_flip_gen>;
using coupled_runner_t = cadmium::engine::stdevs::runner<float, gen_proc_coupled>;

// ── Atomic runner tests ───────────────────────────────────────────────────────

SCENARIO("stdevs atomic runner first event is at period_a regardless of seed",
         "[stdevs][runner][atomic]") {
    GIVEN("an int_flip_gen runner initialised at t=0 with seed 42") {
        atomic_runner_t r(0.0f, 42u);
        WHEN("run_until(0.5) is called (before first event at t=1)") {
            float t_end = r.run_until(0.5f);
            THEN("runner stops at t=1 (first scheduled event)") {
                CHECK(t_end == 1.0f);
            }
        }
    }
}

SCENARIO("stdevs atomic runner advances past the requested end time", "[stdevs][runner][atomic]") {
    GIVEN("an int_flip_gen runner seeded with 1") {
        atomic_runner_t r(0.0f, 1u);
        WHEN("run_until(10.0) is called") {
            float t_end = r.run_until(10.0f);
            THEN("returned time is >= 10.0") {
                CHECK(t_end >= 10.0f);
            }
        }
    }
}

SCENARIO("stdevs atomic runner exact replay: same seed produces same result",
         "[stdevs][runner][atomic][determinism]") {
    GIVEN("two int_flip_gen runners seeded identically with 77") {
        atomic_runner_t r1(0.0f, 77u);
        atomic_runner_t r2(0.0f, 77u);
        WHEN("both are run to t=50") {
            float t1 = r1.run_until(50.0f);
            float t2 = r2.run_until(50.0f);
            THEN("both return the same stop time") {
                CHECK(t1 == t2);
            }
        }
    }
}

// ── Coupled runner tests ──────────────────────────────────────────────────────

SCENARIO("stdevs coupled runner initialises and runs without error", "[stdevs][runner][coupled]") {
    GIVEN("a gen_proc coupled runner seeded with 5") {
        coupled_runner_t r(0.0f, 5u);
        WHEN("run_until(10.0) is called") {
            float t_end = r.run_until(10.0f);
            THEN("runner advances past t=10") {
                CHECK(t_end >= 10.0f);
            }
        }
    }
}

SCENARIO("stdevs coupled runner exact replay", "[stdevs][runner][coupled][determinism]") {
    GIVEN("two gen_proc coupled runners seeded identically with 999") {
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

// ── Statistical test: Bernoulli mean period ───────────────────────────────────
//
// int_flip_gen has p=0.5 → expected mean period = (1+2)/2 = 1.5.
// Run until T = 1500 → expect ≈ 1000 events.
// The last event time (returned by run_until) is the first event ≥ 1500.
// With mean=1.5 and N≈1000, 3σ on mean ≈ 3·(σ_period/√N).
// σ_period = sqrt(p(1-p)·(b-a)²) = sqrt(0.25·1) = 0.5; σ_mean ≈ 0.5/√1000 ≈ 0.016.
// The last event time should be within a few periods of 1500 → generous bound [1498, 1504].

SCENARIO("stdevs runner Bernoulli mean period is statistically correct",
         "[stdevs][runner][statistical]") {
    GIVEN("an int_flip_gen runner seeded with 2025, run until t=1500") {
        atomic_runner_t r(0.0f, 2025u);
        float t_end = r.run_until(1500.0f);
        WHEN("the stop time is inspected") {
            THEN("it is within a few periods of 1500 (mean period = 1.5, bound [1498, 1504])") {
                CHECK(t_end >= 1498.0f);
                CHECK(t_end <= 1504.0f);
            }
        }
    }
}
