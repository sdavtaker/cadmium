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
#include <cadmium/engine/stdevs_simulator.hpp>

#include <catch2/catch_test_macros.hpp>
#include <limits>
#include <random>

// ── Concrete submodels used throughout these tests ────────────────────────────
//
// int_flip_gen: period ∈ {1, 2} with equal probability; outputs constant 1.
// unit_processor: always accepts (p_accept=1), processing_time=1.

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
    struct unit_processor
        : public cadmium::basic_models::stdevs::bernoulli_processor<int, TIME, RNG> {
        TIME processing_time() const override {
            return TIME{1};
        }
        double p_accept() const override {
            return 1.0;
        }
    };

} // anonymous namespace

// ── Type aliases ──────────────────────────────────────────────────────────────

using gen_defs  = cadmium::basic_models::stdevs::bernoulli_generator_defs<int>;
using proc_defs = cadmium::basic_models::stdevs::bernoulli_processor_defs<int>;

using gen_sim_t     = cadmium::engine::stdevs::simulator<int_flip_gen, float, RNG>;
using proc_sim_t    = cadmium::engine::stdevs::simulator<unit_processor, float, RNG>;
using proc_in_box_t = typename cadmium::make_message_box<unit_processor<float>::input_ports>::type;

// ── bernoulli_generator simulator ────────────────────────────────────────────

SCENARIO("stdevs bernoulli_generator simulator initialises with next = period_a",
         "[stdevs][simulator][bernoulli_generator]") {
    GIVEN("an int_flip_gen simulator initialised at t=0") {
        RNG rng(42u);
        gen_sim_t s;
        s.init(0.0f, rng);
        WHEN("next is queried") {
            THEN("it equals period_a() = 1") {
                CHECK(s.next() == 1.0f);
            }
        }
    }
}

SCENARIO("stdevs bernoulli_generator simulator produces output at scheduled time",
         "[stdevs][simulator][bernoulli_generator]") {
    GIVEN("an int_flip_gen simulator initialised at t=0") {
        RNG rng(42u);
        gen_sim_t s;
        s.init(0.0f, rng);
        WHEN("outputs are collected at the scheduled time t=1") {
            s.collect_outputs(1.0f);
            THEN("the out port holds the configured output_message() = 1") {
                const auto &opt = cadmium::get_message<gen_defs::out>(s.outbox());
                REQUIRE(opt.has_value());
                CHECK(opt.value() == 1);
            }
        }
    }
}

SCENARIO("stdevs bernoulli_generator simulator produces no output before scheduled time",
         "[stdevs][simulator][bernoulli_generator]") {
    GIVEN("an int_flip_gen simulator initialised at t=0") {
        RNG rng(42u);
        gen_sim_t s;
        s.init(0.0f, rng);
        WHEN("outputs are collected at t=0.5 (before first event)") {
            s.collect_outputs(0.5f);
            THEN("the outbox is empty") {
                CHECK(cadmium::engine::devs::all_box_empty(s.outbox()));
            }
        }
    }
}

SCENARIO(
    "stdevs bernoulli_generator simulator reschedules within {period_a, period_b} after advance",
    "[stdevs][simulator][bernoulli_generator]") {
    GIVEN("an int_flip_gen simulator advanced to t=1") {
        RNG rng(42u);
        gen_sim_t s;
        s.init(0.0f, rng);
        s.advance_simulation(1.0f);
        WHEN("next is queried") {
            THEN("it is 2.0 (period_a=1 from t=1) or 3.0 (period_b=2 from t=1)") {
                float n = s.next();
                CHECK((n == 2.0f || n == 3.0f));
            }
        }
    }
}

SCENARIO("stdevs bernoulli_generator produces identical transitions with same seed",
         "[stdevs][simulator][bernoulli_generator][determinism]") {
    GIVEN("two int_flip_gen simulators seeded identically") {
        RNG rng_a(99u);
        RNG rng_b(99u);
        gen_sim_t sa, sb;
        sa.init(0.0f, rng_a);
        sb.init(0.0f, rng_b);
        WHEN("both are advanced through 10 internal transitions") {
            for (int i = 0; i < 10; ++i) {
                REQUIRE(sa.next() == sb.next());
                float t = sa.next();
                sa.advance_simulation(t);
                sb.advance_simulation(t);
            }
            THEN("next times remain identical at every step") {
                CHECK(sa.next() == sb.next());
            }
        }
    }
}

SCENARIO("stdevs bernoulli_generator period distribution is statistically correct",
         "[stdevs][simulator][bernoulli_generator][statistical]") {
    GIVEN("an int_flip_gen simulator run for 2000 internal transitions") {
        RNG rng(12345u);
        gen_sim_t s;
        s.init(0.0f, rng);
        int period1_count = 0;
        const int N       = 2000;
        for (int i = 0; i < N; ++i) {
            float t = s.next();
            s.advance_simulation(t);
            if (s.next() - t == 1.0f)
                ++period1_count;
        }
        WHEN("the fraction of period_a choices is measured") {
            double frac = static_cast<double>(period1_count) / N;
            THEN("it is within 3 sigma of p=0.5 (sigma≈0.011; bound: [0.47, 0.53])") {
                CHECK(frac >= 0.47);
                CHECK(frac <= 0.53);
            }
        }
    }
}

// ── bernoulli_processor simulator ────────────────────────────────────────────

SCENARIO("stdevs bernoulli_processor simulator starts passive",
         "[stdevs][simulator][bernoulli_processor]") {
    GIVEN("a unit_processor simulator initialised at t=0") {
        RNG rng(1u);
        proc_sim_t s;
        s.init(0.0f, rng);
        WHEN("next is queried") {
            THEN("it is infinity (idle, no job)") {
                CHECK(s.next() == std::numeric_limits<float>::infinity());
            }
        }
    }
}

SCENARIO("stdevs bernoulli_processor schedules output after accepting a job",
         "[stdevs][simulator][bernoulli_processor]") {
    GIVEN("a unit_processor simulator initialised at t=0") {
        RNG rng(1u);
        proc_sim_t s;
        s.init(0.0f, rng);
        WHEN("a message is placed in the inbox and simulation is advanced at t=2") {
            proc_in_box_t box{};
            cadmium::get_message<proc_defs::in>(box) = 42;
            s.inbox(box);
            s.advance_simulation(2.0f);
            THEN("next is t=2 + processing_time=1 = 3") {
                CHECK(s.next() == 3.0f);
            }
        }
    }
}

SCENARIO("stdevs bernoulli_processor outputs the accepted job at scheduled time",
         "[stdevs][simulator][bernoulli_processor]") {
    GIVEN("a unit_processor that accepted job=42 at t=2 and fires at t=3") {
        RNG rng(1u);
        proc_sim_t s;
        s.init(0.0f, rng);
        proc_in_box_t box{};
        cadmium::get_message<proc_defs::in>(box) = 42;
        s.inbox(box);
        s.advance_simulation(2.0f);
        WHEN("outputs are collected at t=3") {
            s.collect_outputs(3.0f);
            THEN("the out port holds 42") {
                const auto &opt = cadmium::get_message<proc_defs::out>(s.outbox());
                REQUIRE(opt.has_value());
                CHECK(opt.value() == 42);
            }
        }
    }
}

SCENARIO("stdevs bernoulli_processor returns to idle after internal transition",
         "[stdevs][simulator][bernoulli_processor]") {
    GIVEN("a unit_processor that has just completed a job at t=3") {
        RNG rng(1u);
        proc_sim_t s;
        s.init(0.0f, rng);
        proc_in_box_t box{};
        cadmium::get_message<proc_defs::in>(box) = 7;
        s.inbox(box);
        s.advance_simulation(1.0f);
        s.advance_simulation(2.0f); // internal transition fires
        WHEN("next is queried") {
            THEN("it is infinity (back to idle)") {
                CHECK(s.next() == std::numeric_limits<float>::infinity());
            }
        }
    }
}

// ── Error conditions ──────────────────────────────────────────────────────────

SCENARIO("stdevs simulator rejects collect_outputs past next event", "[stdevs][simulator][error]") {
    GIVEN("a gen simulator with next = 1.0") {
        RNG rng(1u);
        gen_sim_t s;
        s.init(0.0f, rng);
        WHEN("collect_outputs is called at t=2 (past next=1)") {
            THEN("a domain_error is thrown") {
                CHECK_THROWS_AS(s.collect_outputs(2.0f), std::domain_error);
            }
        }
    }
}

SCENARIO("stdevs simulator rejects advance_simulation in the past", "[stdevs][simulator][error]") {
    GIVEN("a gen simulator advanced to t=1") {
        RNG rng(1u);
        gen_sim_t s;
        s.init(0.0f, rng);
        s.advance_simulation(1.0f);
        WHEN("advance_simulation is called at t=0.5") {
            THEN("a domain_error is thrown") {
                CHECK_THROWS_AS(s.advance_simulation(0.5f), std::domain_error);
            }
        }
    }
}

SCENARIO("stdevs simulator rejects advance_simulation past a scheduled event",
         "[stdevs][simulator][error]") {
    GIVEN("a gen simulator with next = 1.0") {
        RNG rng(1u);
        gen_sim_t s;
        s.init(0.0f, rng);
        WHEN("advance_simulation is called at t=5 (past next=1)") {
            THEN("a domain_error is thrown") {
                CHECK_THROWS_AS(s.advance_simulation(5.0f), std::domain_error);
            }
        }
    }
}
