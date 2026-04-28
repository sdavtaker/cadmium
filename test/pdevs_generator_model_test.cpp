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

#include <catch2/catch_test_macros.hpp>
#include <stdexcept>

#include <cadmium/basic_model/pdevs/generator.hpp>
#include <cadmium/concept/concept_helpers.hpp>
#include <cadmium/modeling/message_bag.hpp>

const float init_period = 0.1f;
const float init_output_message = 1.0f;

template<typename TIME>
using floating_generator_base = cadmium::basic_models::pdevs::generator<float, TIME>;
using floating_generator_defs = cadmium::basic_models::pdevs::generator_defs<float>;

template<typename TIME>
struct floating_generator : public floating_generator_base<TIME> {
    float period() const override { return init_period; }
    float output_message() const override { return init_output_message; }
};

TEST_CASE("pdevs generator is atomic", "[pdevs][generator]") {
    CHECK(cadmium::model_checks::is_atomic<floating_generator>::value());
}

TEST_CASE("pdevs generator is constructable", "[pdevs][generator]") {
    CHECK_NOTHROW(floating_generator<float>{});
}

TEST_CASE("pdevs generator time advance is constant across internal transitions", "[pdevs][generator]") {
    auto g = floating_generator<float>();
    CHECK(g.time_advance() == init_period);
    g.internal_transition();
    CHECK(g.time_advance() == init_period);
}

TEST_CASE("pdevs generator throws on confluence transition", "[pdevs][generator]") {
    auto g = floating_generator<float>();
    typename cadmium::make_message_bags<floating_generator<float>::input_ports>::type bags;
    CHECK_THROWS_AS(g.confluence_transition(5.0, bags), std::logic_error);
}

TEST_CASE("pdevs generator throws on external transition", "[pdevs][generator]") {
    auto g = floating_generator<float>();
    typename cadmium::make_message_bags<floating_generator<float>::input_ports>::type bags;
    CHECK_THROWS_AS(g.external_transition(5.0, bags), std::logic_error);
}

TEST_CASE("pdevs generator output returns configured message", "[pdevs][generator]") {
    auto g = floating_generator<float>();
    auto o = g.output();
    auto o_m = cadmium::get_messages<floating_generator_defs::out>(o);
    REQUIRE(o_m.size() == 1);
    CHECK(o_m.at(0) == init_output_message);
}
