/**
 * Copyright (c) 2013-present, Damian Vicino
 * Carleton University, Universite de Nice-Sophia Antipolis
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

#include <cadmium/basic_model/pdevs/accumulator.hpp>
#include <cadmium/basic_model/pdevs/generator.hpp>
#include <cadmium/engine/pdevs_simulator.hpp>
#include <cadmium/logger/tuple_to_ostream.hpp>

#include <catch2/catch_test_macros.hpp>
#include <limits>

template <typename TIME>
using int_accumulator      = cadmium::basic_models::pdevs::accumulator<int, TIME>;
using int_accumulator_defs = cadmium::basic_models::pdevs::accumulator_defs<int>;

using sim_t        = cadmium::engine::simulator<int_accumulator, float>;
using input_ports  = int_accumulator<float>::input_ports;
using input_bags_t = typename cadmium::make_message_bags<input_ports>::type;

SCENARIO("accumulator simulator starts in passive state", "[pdevs][simulator][accumulator]") {
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

SCENARIO("accumulator simulator receiving only add messages stays passive",
         "[pdevs][simulator][accumulator]") {
    GIVEN("a freshly initialised accumulator simulator") {
        sim_t s;
        s.init(0.0f);
        WHEN("messages are added on the add port and simulation is advanced") {
            input_bags_t bags{};
            cadmium::get_messages<int_accumulator_defs::add>(bags).assign({1, 2, 3, 4});
            s.inbox(bags);
            s.advance_simulation(3.0f);
            THEN("next remains infinity because no reset was received") {
                CHECK(s.next() == std::numeric_limits<float>::infinity());
            }
        }
    }
}

SCENARIO("accumulator simulator schedules internal transition when reset is "
         "received",
         "[pdevs][simulator][accumulator]") {
    GIVEN("an accumulator that has received add messages and is then reset") {
        sim_t s;
        s.init(0.0f);
        input_bags_t bags{};
        cadmium::get_messages<int_accumulator_defs::add>(bags).assign({1, 2, 3, 4});
        s.inbox(bags);
        s.advance_simulation(3.0f);
        WHEN("a reset message arrives at time 4") {
            input_bags_t reset_bags{};
            cadmium::get_messages<int_accumulator_defs::reset>(reset_bags).emplace_back();
            s.inbox(reset_bags);
            s.advance_simulation(4.0f);
            THEN("next is scheduled at time 4 for the internal transition") {
                CHECK(s.next() == 4.0f);
            }
        }
    }
}

SCENARIO("accumulator simulator output is the sum of all received add messages",
         "[pdevs][simulator][accumulator]") {
    GIVEN("an accumulator that has accumulated 1+2+3+4 and is then reset") {
        sim_t s;
        s.init(0.0f);
        input_bags_t bags{};
        cadmium::get_messages<int_accumulator_defs::add>(bags).assign({1, 2, 3, 4});
        s.inbox(bags);
        s.advance_simulation(3.0f);
        input_bags_t reset_bags{};
        cadmium::get_messages<int_accumulator_defs::reset>(reset_bags).emplace_back();
        s.inbox(reset_bags);
        s.advance_simulation(4.0f);
        WHEN("outputs are collected at the scheduled time") {
            s.collect_outputs(4.0f);
            auto o = s.outbox();
            THEN("the sum port contains exactly 10") {
                REQUIRE(!cadmium::engine::all_bags_empty(o));
                REQUIRE(cadmium::get_messages<int_accumulator_defs::sum>(o).size() == 1);
                CHECK(cadmium::get_messages<int_accumulator_defs::sum>(o).at(0) == 10);
            }
        }
    }
}

SCENARIO("accumulator simulator internal transition clears the counter",
         "[pdevs][simulator][accumulator]") {
    GIVEN("an accumulator that was reset once and the internal transition "
          "drained") {
        sim_t s;
        s.init(0.0f);
        input_bags_t reset_bags{};
        cadmium::get_messages<int_accumulator_defs::reset>(reset_bags).emplace_back();
        s.inbox(reset_bags);
        s.advance_simulation(3.0f);
        input_bags_t empty{};
        s.inbox(empty);
        s.advance_simulation(3.0f);
        REQUIRE(s.next() == std::numeric_limits<float>::infinity());
        WHEN("a second reset arrives at time 5 with no prior add messages") {
            input_bags_t bags{};
            cadmium::get_messages<int_accumulator_defs::reset>(bags).emplace_back();
            s.inbox(bags);
            s.advance_simulation(5.0f);
            s.collect_outputs(5.0f);
            auto o = s.outbox();
            THEN("the output sum is 0") {
                REQUIRE(cadmium::get_messages<int_accumulator_defs::sum>(o).size() == 1);
                CHECK(cadmium::get_messages<int_accumulator_defs::sum>(o).at(0) == 0);
            }
        }
    }
}

SCENARIO("accumulator simulator rejects advancing to a time in the past",
         "[pdevs][simulator][accumulator]") {
    GIVEN("an accumulator advanced to time 3 with a pending reset") {
        sim_t s;
        s.init(0.0f);
        input_bags_t bags{};
        cadmium::get_messages<int_accumulator_defs::add>(bags).assign({1, 2, 3, 4});
        cadmium::get_messages<int_accumulator_defs::reset>(bags).emplace_back();
        s.inbox(bags);
        s.advance_simulation(3.0f);
        WHEN("advance_simulation is called with time 2 (in the past)") {
            s.inbox(bags);
            THEN("a domain_error is thrown") {
                CHECK_THROWS_AS(s.advance_simulation(2.0f), std::domain_error);
            }
        }
    }
}

SCENARIO("accumulator simulator rejects advancing past a scheduled internal event",
         "[pdevs][simulator][accumulator]") {
    GIVEN("an accumulator at time 3 with an internal event scheduled at time 3") {
        sim_t s;
        s.init(0.0f);
        input_bags_t bags{};
        cadmium::get_messages<int_accumulator_defs::add>(bags).assign({1, 2, 3, 4});
        cadmium::get_messages<int_accumulator_defs::reset>(bags).emplace_back();
        s.inbox(bags);
        s.advance_simulation(3.0f);
        WHEN("advance_simulation is called with time 4 (past the scheduled event)") {
            s.inbox(bags);
            THEN("a domain_error is thrown") {
                CHECK_THROWS_AS(s.advance_simulation(4.0f), std::domain_error);
            }
        }
    }
}

// Generator simulator tests

const float init_period         = 1.0f;
const float init_output_message = 2.0f;

template <typename TIME>
using floating_generator_base = cadmium::basic_models::pdevs::generator<float, TIME>;
using floating_generator_defs = cadmium::basic_models::pdevs::generator_defs<float>;

template <typename TIME> struct floating_generator : public floating_generator_base<TIME> {
    float period() const override {
        return init_period;
    }
    float output_message() const override {
        return init_output_message;
    }
};

using gen_sim_t = cadmium::engine::simulator<floating_generator, float>;

SCENARIO("generator simulator schedules first output one period after "
         "initialisation",
         "[pdevs][simulator][generator]") {
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

SCENARIO("generator simulator produces no output before the scheduled time",
         "[pdevs][simulator][generator]") {
    GIVEN("a generator simulator initialised at time 0") {
        gen_sim_t s;
        s.init(0.0f);
        WHEN("outputs are collected at time 0.5 (before the first scheduled tick)") {
            s.collect_outputs(0.5f);
            THEN("the outbox is empty") {
                CHECK(cadmium::engine::all_bags_empty(s.outbox()));
            }
        }
    }
}

SCENARIO("generator simulator produces the configured output at the scheduled time",
         "[pdevs][simulator][generator]") {
    GIVEN("a generator simulator initialised at time 0") {
        gen_sim_t s;
        s.init(0.0f);
        WHEN("outputs are collected at the scheduled time of 1.0") {
            s.collect_outputs(1.0f);
            auto out = s.outbox();
            THEN("the output bag contains exactly the configured message") {
                REQUIRE(cadmium::get_messages<floating_generator_defs::out>(out).size() == 1);
                CHECK(cadmium::get_messages<floating_generator_defs::out>(out)[0] == 2.0f);
            }
        }
    }
}

SCENARIO("generator simulator reschedules the next output after advancing "
         "simulation",
         "[pdevs][simulator][generator]") {
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
