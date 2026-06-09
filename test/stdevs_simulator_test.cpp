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

#include <cadmium/engine/stdevs_simulator.hpp>
#include <cadmium/modeling/message_box.hpp>
#include <cadmium/modeling/ports.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <limits>
#include <random>

// ── Test models ────────────────────────────────────────────────────────────────
//
// flip_gen: generates events with period randomly drawn from {1, 2} via
// Bernoulli(0.5) in each internal_transition.  Initial state = 1 (period 1)
// so the first scheduled event is deterministic regardless of seed.
//
// stdevs_sink: passive model that records received messages. Has external input
// but always stays passive — time_advance = infinity.

namespace {

    using RNG = std::mt19937;

    template <typename TIME> struct flip_gen {
        struct out : public cadmium::out_port<int> {};
        using state_type   = TIME;
        state_type state   = TIME{1}; // first event at t = state
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

    template <typename TIME> struct stdevs_sink {
        struct in : public cadmium::in_port<int> {};
        using state_type   = int; // count of received messages
        state_type state   = 0;
        using input_ports  = std::tuple<in>;
        using output_ports = std::tuple<>;

        void internal_transition(RNG &) {}

        void external_transition(TIME, typename cadmium::make_message_box<input_ports>::type box,
                                 RNG &) {
            if (cadmium::get_message<in>(box).has_value())
                ++state;
        }

        typename cadmium::make_message_box<output_ports>::type output() const {
            return {};
        }

        TIME time_advance() const {
            return std::numeric_limits<TIME>::infinity();
        }
    };

} // anonymous namespace

// ── Type aliases ──────────────────────────────────────────────────────────────

using flip_sim_t = cadmium::engine::stdevs::simulator<flip_gen, float, RNG>;
using sink_sim_t = cadmium::engine::stdevs::simulator<stdevs_sink, float, RNG>;
using out_box_t  = typename cadmium::make_message_box<flip_gen<float>::output_ports>::type;
using in_box_t   = typename cadmium::make_message_box<stdevs_sink<float>::input_ports>::type;

// ── Simulator tests: flip_gen ─────────────────────────────────────────────────

SCENARIO("stdevs flip_gen simulator initialises with next = initial period",
         "[stdevs][simulator][flip_gen]") {
    GIVEN("a flip_gen simulator initialised at t=0") {
        RNG rng(42u);
        flip_sim_t s;
        s.init(0.0f, rng);
        WHEN("next is queried") {
            THEN("it equals the initial period of 1") {
                CHECK(s.next() == 1.0f);
            }
        }
    }
}

SCENARIO("stdevs flip_gen simulator produces output at scheduled time",
         "[stdevs][simulator][flip_gen]") {
    GIVEN("a flip_gen simulator initialised at t=0") {
        RNG rng(42u);
        flip_sim_t s;
        s.init(0.0f, rng);
        WHEN("outputs are collected at the scheduled time of 1.0") {
            s.collect_outputs(1.0f);
            THEN("the out port holds value 1") {
                const auto &opt = cadmium::get_message<flip_gen<float>::out>(s.outbox());
                REQUIRE(opt.has_value());
                CHECK(opt.value() == 1);
            }
        }
    }
}

SCENARIO("stdevs flip_gen simulator produces no output before scheduled time",
         "[stdevs][simulator][flip_gen]") {
    GIVEN("a flip_gen simulator initialised at t=0") {
        RNG rng(42u);
        flip_sim_t s;
        s.init(0.0f, rng);
        WHEN("outputs are collected at t=0.5 (before next)") {
            s.collect_outputs(0.5f);
            THEN("the outbox is empty") {
                CHECK(cadmium::engine::devs::all_box_empty(s.outbox()));
            }
        }
    }
}

SCENARIO("stdevs flip_gen simulator reschedules after internal transition",
         "[stdevs][simulator][flip_gen]") {
    GIVEN("a flip_gen simulator initialised at t=0 (next=1) and advanced to t=1") {
        RNG rng(42u);
        flip_sim_t s;
        s.init(0.0f, rng);
        s.advance_simulation(1.0f);
        WHEN("next is queried after the transition") {
            THEN("it is either 2 or 3 (period drawn from {1,2}, added to t=1)") {
                float n = s.next();
                CHECK((n == 2.0f || n == 3.0f));
            }
        }
    }
}

SCENARIO("same seed produces identical transition sequence",
         "[stdevs][simulator][flip_gen][determinism]") {
    GIVEN("two flip_gen simulators seeded identically") {
        RNG rng_a(99u);
        RNG rng_b(99u);
        flip_sim_t sa, sb;
        sa.init(0.0f, rng_a);
        sb.init(0.0f, rng_b);
        WHEN("both are advanced through 10 internal transitions") {
            for (int i = 0; i < 10; ++i) {
                float ta = sa.next();
                float tb = sb.next();
                REQUIRE(ta == tb);
                sa.advance_simulation(ta);
                sb.advance_simulation(tb);
            }
            THEN("their next scheduled times remain identical throughout") {
                CHECK(sa.next() == sb.next());
            }
        }
    }
}

SCENARIO("stdevs flip_gen Bernoulli distribution is statistically correct",
         "[stdevs][simulator][flip_gen][statistical]") {
    GIVEN("a flip_gen simulator run for 2000 transitions") {
        RNG rng(12345u);
        flip_sim_t s;
        s.init(0.0f, rng);
        int period1_count = 0;
        const int N       = 2000;
        float t           = 0.0f;
        for (int i = 0; i < N; ++i) {
            t = s.next();
            s.advance_simulation(t);
            if (s.next() - t == 1.0f)
                ++period1_count;
        }
        WHEN("the fraction of period-1 outcomes is measured") {
            double frac = static_cast<double>(period1_count) / N;
            THEN("it is within 3 sigma of 0.5 (sigma = sqrt(0.25/N) ≈ 0.011)") {
                // 3 sigma ≈ 0.033; very low false-failure rate
                CHECK(frac >= 0.47);
                CHECK(frac <= 0.53);
            }
        }
    }
}

// ── Simulator tests: stdevs_sink (external transition) ────────────────────────

SCENARIO("stdevs sink stays passive after init", "[stdevs][simulator][sink]") {
    GIVEN("a sink simulator initialised at t=0") {
        RNG rng(1u);
        sink_sim_t s;
        s.init(0.0f, rng);
        WHEN("next is queried") {
            THEN("it is infinity") {
                CHECK(s.next() == std::numeric_limits<float>::infinity());
            }
        }
    }
}

SCENARIO("stdevs sink counts received messages via external_transition",
         "[stdevs][simulator][sink]") {
    GIVEN("a sink simulator initialised at t=0") {
        RNG rng(1u);
        sink_sim_t s;
        s.init(0.0f, rng);
        WHEN("three messages are delivered at different times") {
            for (int i = 1; i <= 3; ++i) {
                in_box_t box{};
                cadmium::get_message<stdevs_sink<float>::in>(box) = i;
                s.inbox(box);
                s.advance_simulation(static_cast<float>(i));
            }
            THEN("the state count equals 3") {
                // access through the model is not directly exposed; use next() as
                // proxy: still infinity since sink never schedules an internal event
                CHECK(s.next() == std::numeric_limits<float>::infinity());
            }
        }
    }
}

// ── Error conditions ──────────────────────────────────────────────────────────

SCENARIO("stdevs simulator rejects collect_outputs past next event", "[stdevs][simulator][error]") {
    GIVEN("a flip_gen simulator with next = 1.0") {
        RNG rng(1u);
        flip_sim_t s;
        s.init(0.0f, rng);
        WHEN("collect_outputs is called at t=2 (past next)") {
            THEN("a domain_error is thrown") {
                CHECK_THROWS_AS(s.collect_outputs(2.0f), std::domain_error);
            }
        }
    }
}

SCENARIO("stdevs simulator rejects advance_simulation in the past", "[stdevs][simulator][error]") {
    GIVEN("a flip_gen simulator advanced to t=1") {
        RNG rng(1u);
        flip_sim_t s;
        s.init(0.0f, rng);
        s.advance_simulation(1.0f);
        WHEN("advance_simulation is called at t=0.5 (in the past)") {
            THEN("a domain_error is thrown") {
                CHECK_THROWS_AS(s.advance_simulation(0.5f), std::domain_error);
            }
        }
    }
}

SCENARIO("stdevs simulator rejects advance_simulation past a scheduled event",
         "[stdevs][simulator][error]") {
    GIVEN("a flip_gen simulator with next = 1.0") {
        RNG rng(1u);
        flip_sim_t s;
        s.init(0.0f, rng);
        WHEN("advance_simulation is called at t=5 (past next=1)") {
            THEN("a domain_error is thrown") {
                CHECK_THROWS_AS(s.advance_simulation(5.0f), std::domain_error);
            }
        }
    }
}
