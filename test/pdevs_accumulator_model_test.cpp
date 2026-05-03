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
#include <cadmium/concepts/pdevs_concepts.hpp>
#include <cadmium/modeling/message_bag.hpp>

#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <stdexcept>

template <typename TIME>
using floating_accumulator      = cadmium::basic_models::pdevs::accumulator<float, TIME>;
using floating_accumulator_defs = cadmium::basic_models::pdevs::accumulator_defs<float>;

SCENARIO("pdevs accumulator model satisfies the atomic model concept", "[pdevs][accumulator]") {
    GIVEN("the floating_accumulator model type") {
        WHEN("the atomic concept check is evaluated") {
            THEN("it passes") {
                CHECK(cadmium::concepts::pdevs::AtomicModel<floating_accumulator<float>, float>);
            }
        }
    }
}

SCENARIO("pdevs accumulator model can be default-constructed", "[pdevs][accumulator]") {
    GIVEN("no preconditions") {
        WHEN("a floating_accumulator is default-constructed") {
            THEN("no exception is thrown") {
                CHECK_NOTHROW(floating_accumulator<float>{});
            }
        }
    }
}

SCENARIO("pdevs accumulator internal transition clears accumulated value", "[pdevs][accumulator]") {
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

SCENARIO("pdevs accumulator internal transition is rejected when not in "
         "reset-pending state",
         "[pdevs][accumulator]") {
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

SCENARIO("pdevs accumulator external transition is rejected when in "
         "reset-pending state",
         "[pdevs][accumulator]") {
    GIVEN("an accumulator in reset-pending state") {
        auto a  = floating_accumulator<float>();
        a.state = std::make_tuple(1.0f, true);
        WHEN("external_transition is called with an add message") {
            typename cadmium::make_message_bags<floating_accumulator<float>::input_ports>::type
                bags;
            cadmium::get_messages<floating_accumulator_defs::add>(bags).push_back(5.0f);
            THEN("a logic_error is thrown") {
                CHECK_THROWS_AS(a.external_transition(1.0f, bags), std::logic_error);
            }
        }
    }
}

SCENARIO("pdevs accumulator output is rejected when not in reset-pending state",
         "[pdevs][accumulator]") {
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

SCENARIO("pdevs accumulator accumulates a bag of values across transitions "
         "then outputs the sum",
         "[pdevs][accumulator]") {
    GIVEN("an accumulator with initial sum 10.0") {
        auto a  = floating_accumulator<float>();
        a.state = std::make_tuple(10.0f, false);
        WHEN("5 is added, then 3 and 7 are added, then 3 more and a reset arrive") {
            typename cadmium::make_message_bags<floating_accumulator<float>::input_ports>::type
                bags;
            cadmium::get_messages<floating_accumulator_defs::add>(bags).push_back(5.0f);
            a.external_transition(10.0f, bags);
            CHECK(std::get<float>(a.state) == 15.0f);

            typename cadmium::make_message_bags<floating_accumulator<float>::input_ports>::type
                bags2;
            cadmium::get_messages<floating_accumulator_defs::add>(bags2).push_back(3.0f);
            cadmium::get_messages<floating_accumulator_defs::add>(bags2).push_back(7.0f);
            a.external_transition(9.0f, bags2);
            CHECK(std::get<float>(a.state) == 25.0f);

            typename cadmium::make_message_bags<floating_accumulator<float>::input_ports>::type
                bags3;
            cadmium::get_messages<floating_accumulator_defs::add>(bags3).push_back(3.0f);
            cadmium::get_messages<floating_accumulator_defs::reset>(bags3).emplace_back();
            a.external_transition(2.0f, bags3);

            THEN("the accumulated sum is 28 and the model enters reset-pending "
                 "state") {
                CHECK(std::get<float>(a.state) == 28.0f);
                CHECK(std::get<bool>(a.state) == true);
            }
            AND_THEN("output returns a single message with the sum 28") {
                auto out = a.output();
                REQUIRE(cadmium::get_messages<floating_accumulator_defs::sum>(out).size() == 1);
                CHECK(cadmium::get_messages<floating_accumulator_defs::sum>(out)[0] == 28.0f);
            }
        }
    }
}

SCENARIO("pdevs accumulator confluence transition applies internal reset then "
         "accumulates new input",
         "[pdevs][accumulator]") {
    GIVEN("an accumulator with sum 10 that receives a reset message, entering "
          "reset-pending state") {
        auto a  = floating_accumulator<float>();
        a.state = std::make_tuple(10.0f, false);
        typename cadmium::make_message_bags<floating_accumulator<float>::input_ports>::type bags;
        cadmium::get_messages<floating_accumulator_defs::reset>(bags).emplace_back();
        a.external_transition(0.0f, bags);
        REQUIRE(std::get<float>(a.state) == 10.0f);
        REQUIRE(std::get<bool>(a.state) == true);
        WHEN("a confluence transition arrives with an add message of 2") {
            typename cadmium::make_message_bags<floating_accumulator<float>::input_ports>::type
                bags2;
            cadmium::get_messages<floating_accumulator_defs::add>(bags2).push_back(2.0f);
            a.confluence_transition(0.0f, bags2);
            THEN("the internal reset fires first and the new value 2 is accumulated") {
                CHECK(std::get<float>(a.state) == 2.0f);
                CHECK(std::get<bool>(a.state) == false);
            }
        }
    }
}
