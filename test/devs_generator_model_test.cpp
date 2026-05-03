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

#include <cadmium/basic_model/devs/generator.hpp>
#include <cadmium/modeling/message_box.hpp>

#include <catch2/catch_test_macros.hpp>
#include <stdexcept>

const float init_period         = 0.1f;
const float init_output_message = 1.0f;

template <typename TIME>
using floating_generator_base = cadmium::basic_models::devs::generator<float, TIME>;
using floating_generator_defs = cadmium::basic_models::devs::generator_defs<float>;

template <typename TIME> struct floating_generator : public floating_generator_base<TIME> {
    float period() const override {
        return init_period;
    }
    float output_message() const override {
        return init_output_message;
    }
};

SCENARIO("devs generator model satisfies the atomic model concept", "[devs][generator]") {
    GIVEN("the floating_generator model type") {
        WHEN("the atomic concept check is evaluated") {
            THEN("it passes") {
                CHECK(requires { std::declval<floating_generator<float>>().time_advance(); });
            }
        }
    }
}

SCENARIO("devs generator model can be default-constructed", "[devs][generator]") {
    GIVEN("no preconditions") {
        WHEN("a floating_generator is default-constructed") {
            THEN("no exception is thrown") {
                CHECK_NOTHROW(floating_generator<float>{});
            }
        }
    }
}

SCENARIO("devs generator model time advance is stable under internal transitions",
         "[devs][generator]") {
    GIVEN("a freshly constructed generator") {
        auto g = floating_generator<float>();
        WHEN("time_advance is checked before and after an internal transition") {
            THEN("it equals the configured period both times") {
                CHECK(g.time_advance() == init_period);
                g.internal_transition();
                CHECK(g.time_advance() == init_period);
            }
        }
    }
}

SCENARIO("devs generator model rejects external transitions", "[devs][generator]") {
    GIVEN("a freshly constructed generator") {
        auto g = floating_generator<float>();
        WHEN("external_transition is called") {
            typename cadmium::make_message_box<floating_generator<float>::input_ports>::type input;
            THEN("a logic_error is thrown") {
                CHECK_THROWS_AS(g.external_transition(5.0, input), std::logic_error);
            }
        }
    }
}

SCENARIO("devs generator model output produces the configured message", "[devs][generator]") {
    GIVEN("a freshly constructed generator") {
        auto g = floating_generator<float>();
        WHEN("output is called") {
            auto o   = g.output();
            auto o_m = cadmium::get_message<floating_generator_defs::out>(o);
            THEN("the output port carries the configured message value") {
                REQUIRE(o_m.has_value());
                CHECK(o_m.value() == init_output_message);
            }
        }
    }
}
