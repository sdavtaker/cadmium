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

#include <cadmium/basic_model/pdevs/passive.hpp>
#include <cadmium/concepts/pdevs_concepts.hpp>
#include <cadmium/modeling/message_bag.hpp>

#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <stdexcept>

template <typename TIME>
using floating_passive      = cadmium::basic_models::pdevs::passive<float, TIME>;
using floating_passive_defs = cadmium::basic_models::pdevs::passive_defs<float>;

SCENARIO("pdevs passive model satisfies the atomic model concept", "[pdevs][passive]") {
    GIVEN("the floating_passive model type") {
        WHEN("the atomic concept check is evaluated") {
            THEN("it passes") {
                CHECK(cadmium::concepts::pdevs::AtomicModel<floating_passive<float>, float>);
            }
        }
    }
}

SCENARIO("pdevs passive model can be default-constructed", "[pdevs][passive]") {
    GIVEN("no preconditions") {
        WHEN("a floating_passive is default-constructed") {
            THEN("no exception is thrown") {
                CHECK_NOTHROW(floating_passive<float>{});
            }
        }
    }
}

SCENARIO("pdevs passive model rejects an internal transition", "[pdevs][passive]") {
    GIVEN("a default-constructed passive model") {
        auto p = floating_passive<float>();
        WHEN("internal_transition is called") {
            THEN("a logic_error is thrown") {
                CHECK_THROWS_AS(p.internal_transition(), std::logic_error);
            }
        }
    }
}

SCENARIO("pdevs passive model rejects a confluence transition", "[pdevs][passive]") {
    GIVEN("a default-constructed passive model") {
        auto p = floating_passive<float>();
        WHEN("confluence_transition is called with a message") {
            typename cadmium::make_message_bags<floating_passive<float>::input_ports>::type bags;
            cadmium::get_messages<floating_passive_defs::in>(bags).push_back(1);
            THEN("a logic_error is thrown") {
                CHECK_THROWS_AS(p.confluence_transition(5.0, bags), std::logic_error);
            }
        }
    }
}

SCENARIO("pdevs passive model rejects an output call", "[pdevs][passive]") {
    GIVEN("a default-constructed passive model") {
        auto p = floating_passive<float>();
        WHEN("output is called") {
            THEN("a logic_error is thrown") {
                CHECK_THROWS_AS(p.output(), std::logic_error);
            }
        }
    }
}

SCENARIO("pdevs passive model accepts external input and remains passive", "[pdevs][passive]") {
    GIVEN("a passive model with infinite time advance") {
        auto p = floating_passive<float>();
        REQUIRE(std::isinf(p.time_advance()));
        WHEN("an external transition is applied with a message on the input port") {
            typename cadmium::make_message_bags<floating_passive<float>::input_ports>::type bags;
            cadmium::get_messages<floating_passive_defs::in>(bags).push_back(1);
            THEN("no exception is thrown and time advance remains infinite") {
                CHECK_NOTHROW(p.external_transition(5.0, bags));
                CHECK(std::isinf(p.time_advance()));
            }
        }
    }
}
