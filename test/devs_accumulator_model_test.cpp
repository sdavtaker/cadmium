/**
 * Copyright (c) 2013-present, Damian Vicino
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
#include <cadmium/modeling/message_box.hpp>

#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <stdexcept>

template <typename TIME>
using floating_accumulator      = cadmium::basic_models::devs::accumulator<float, TIME>;
using floating_accumulator_defs = cadmium::basic_models::devs::accumulator_defs<float>;

SCENARIO("devs accumulator model satisfies the atomic model concept", "[devs][accumulator]") {
    GIVEN("the floating_accumulator model type") {
        WHEN("the atomic concept check is evaluated") {
            THEN("it passes") {
                CHECK(requires { std::declval<floating_accumulator<float>>().time_advance(); });
            }
        }
    }
}

SCENARIO("devs accumulator model can be default-constructed", "[devs][accumulator]") {
    GIVEN("no preconditions") {
        WHEN("a floating_accumulator is default-constructed") {
            THEN("no exception is thrown") {
                CHECK_NOTHROW(floating_accumulator<float>{});
            }
        }
    }
}

SCENARIO("devs accumulator internal transition clears the accumulated value",
         "[devs][accumulator]") {
    GIVEN("an accumulator in reset-pending state with sum 1.0") {
        auto a  = floating_accumulator<float>();
        a.state = std::make_tuple(1.0f, true);
        REQUIRE(a.time_advance() == 0.0f);
        WHEN("internal_transition is called") {
            a.internal_transition();
            THEN("the sum is zeroed, the reset flag is cleared, and time advance "
                 "becomes infinite") {
                CHECK(std::isinf(a.time_advance()));
                CHECK(std::get<float>(a.state) == 0.0f);
                CHECK(std::get<bool>(a.state) == false);
            }
        }
    }
}

SCENARIO("devs accumulator internal transition is rejected when not in "
         "reset-pending state",
         "[devs][accumulator]") {
    GIVEN("an accumulator not in reset-pending state") {
        auto a  = floating_accumulator<float>();
        a.state = std::make_tuple(1.0f, false);
        WHEN("internal_transition is called") {
            THEN("a logic_error is thrown") {
                CHECK_THROWS_AS(a.internal_transition(), std::logic_error);
            }
        }
    }
}

SCENARIO("devs accumulator external transition is rejected when in "
         "reset-pending state",
         "[devs][accumulator]") {
    GIVEN("an accumulator in reset-pending state") {
        auto a  = floating_accumulator<float>();
        a.state = std::make_tuple(1.0f, true);
        WHEN("external_transition is called with an add message") {
            typename cadmium::make_message_box<floating_accumulator<float>::input_ports>::type box;
            cadmium::get_message<floating_accumulator_defs::add>(box).emplace(5.0f);
            THEN("a logic_error is thrown") {
                CHECK_THROWS_AS(a.external_transition(1.0f, box), std::logic_error);
            }
        }
    }
}

SCENARIO("devs accumulator output is rejected when not in reset-pending state",
         "[devs][accumulator]") {
    GIVEN("an accumulator not in reset-pending state") {
        auto a  = floating_accumulator<float>();
        a.state = std::make_tuple(1.0f, false);
        WHEN("output is called") {
            THEN("a logic_error is thrown") {
                CHECK_THROWS_AS(a.output(), std::logic_error);
            }
        }
    }
}

SCENARIO("devs accumulator accumulates values across external transitions then "
         "outputs the sum",
         "[devs][accumulator]") {
    GIVEN("an accumulator with initial sum 10.0") {
        auto a  = floating_accumulator<float>();
        a.state = std::make_tuple(10.0f, false);
        WHEN("values 5, 3, 7 are added and then a reset is received") {
            typename cadmium::make_message_box<floating_accumulator<float>::input_ports>::type
                input;
            cadmium::get_message<floating_accumulator_defs::add>(input).emplace(5.0f);
            a.external_transition(10.0f, input);
            CHECK(std::get<float>(a.state) == 15.0f);

            cadmium::get_message<floating_accumulator_defs::add>(input).emplace(3.0f);
            a.external_transition(9.0f, input);
            CHECK(std::get<float>(a.state) == 18.0f);

            cadmium::get_message<floating_accumulator_defs::add>(input).emplace(7.0f);
            a.external_transition(9.0f, input);
            CHECK(std::get<float>(a.state) == 25.0f);

            cadmium::get_message<floating_accumulator_defs::add>(input).emplace(3.0f);
            cadmium::get_message<floating_accumulator_defs::reset>(input).emplace();
            a.external_transition(2.0f, input);

            THEN("the accumulated sum is 28 and the model enters reset-pending "
                 "state") {
                CHECK(std::get<float>(a.state) == 28.0f);
                CHECK(std::get<bool>(a.state) == true);
            }
            AND_THEN("output returns the accumulated sum of 28") {
                auto out = a.output();
                REQUIRE(cadmium::get_message<floating_accumulator_defs::sum>(out).has_value());
                CHECK(cadmium::get_message<floating_accumulator_defs::sum>(out).value() == 28.0f);
            }
        }
    }
}
