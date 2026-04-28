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

#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <stdexcept>

#include <cadmium/basic_model/devs/accumulator.hpp>
#include <cadmium/concept/concept_helpers.hpp>
#include <cadmium/modeling/message_box.hpp>

template<typename TIME>
using floating_accumulator = cadmium::basic_models::devs::accumulator<float, TIME>;
using floating_accumulator_defs = cadmium::basic_models::devs::accumulator_defs<float>;

TEST_CASE("devs accumulator is atomic", "[devs][accumulator]") {
    CHECK(cadmium::model_checks::is_atomic<floating_accumulator>::value());
}

TEST_CASE("devs accumulator is constructable", "[devs][accumulator]") {
    CHECK_NOTHROW(floating_accumulator<float>{});
}

TEST_CASE("devs accumulator internal transition resets state and makes time advance infinite",
          "[devs][accumulator]") {
    auto a = floating_accumulator<float>();
    a.state = std::make_tuple(1.0f, true);
    CHECK(a.time_advance() == 0.0f);
    a.internal_transition();
    CHECK(std::isinf(a.time_advance()));
    CHECK(std::get<float>(a.state) == 0.0f);
    CHECK(std::get<bool>(a.state) == false);
}

TEST_CASE("devs accumulator internal transition throws when not in reset state", "[devs][accumulator]") {
    auto a = floating_accumulator<float>();
    a.state = std::make_tuple(1.0f, false);
    CHECK_THROWS_AS(a.internal_transition(), std::logic_error);
}

TEST_CASE("devs accumulator external transition throws when in reset state", "[devs][accumulator]") {
    auto a = floating_accumulator<float>();
    a.state = std::make_tuple(1.0f, true);
    typename cadmium::make_message_box<floating_accumulator<float>::input_ports>::type box;
    cadmium::get_message<floating_accumulator_defs::add>(box).emplace(5.0f);
    CHECK_THROWS_AS(a.external_transition(1.0f, box), std::logic_error);
}

TEST_CASE("devs accumulator output throws when not in reset state", "[devs][accumulator]") {
    auto a = floating_accumulator<float>();
    a.state = std::make_tuple(1.0f, false);
    CHECK_THROWS_AS(a.output(), std::logic_error);
}

TEST_CASE("devs accumulator output returns accumulated value", "[devs][accumulator]") {
    auto a = floating_accumulator<float>();
    a.state = std::make_tuple(10.0f, false);

    typename cadmium::make_message_box<floating_accumulator<float>::input_ports>::type input;
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
    CHECK(std::get<float>(a.state) == 28.0f);
    CHECK(std::get<bool>(a.state) == true);

    auto out = a.output();
    REQUIRE(cadmium::get_message<floating_accumulator_defs::sum>(out).has_value());
    CHECK(cadmium::get_message<floating_accumulator_defs::sum>(out).value() == 28.0f);
}
