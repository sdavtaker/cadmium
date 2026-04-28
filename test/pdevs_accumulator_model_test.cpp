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
#include <cmath>
#include <stdexcept>

#include <cadmium/basic_model/pdevs/accumulator.hpp>
#include <cadmium/concept/concept_helpers.hpp>
#include <cadmium/modeling/message_bag.hpp>

template<typename TIME>
using floating_accumulator = cadmium::basic_models::pdevs::accumulator<float, TIME>;
using floating_accumulator_defs = cadmium::basic_models::pdevs::accumulator_defs<float>;

TEST_CASE("pdevs accumulator is atomic", "[pdevs][accumulator]") {
    CHECK(cadmium::model_checks::is_atomic<floating_accumulator>::value());
}

TEST_CASE("pdevs accumulator is constructable", "[pdevs][accumulator]") {
    CHECK_NOTHROW(floating_accumulator<float>{});
}

TEST_CASE("pdevs accumulator internal transition resets state and makes time advance infinite",
          "[pdevs][accumulator]") {
    auto a = floating_accumulator<float>();
    a.state = std::make_tuple(1.0f, true);
    CHECK(a.time_advance() == 0.0f);
    a.internal_transition();
    CHECK(std::isinf(a.time_advance()));
    CHECK(std::get<float>(a.state) == 0.0f);
    CHECK(std::get<bool>(a.state) == false);
}

TEST_CASE("pdevs accumulator internal transition throws when not in reset state", "[pdevs][accumulator]") {
    auto a = floating_accumulator<float>();
    a.state = std::make_tuple(1.0f, false);
    CHECK_THROWS_AS(a.internal_transition(), std::logic_error);
}

TEST_CASE("pdevs accumulator external transition throws when in reset state", "[pdevs][accumulator]") {
    auto a = floating_accumulator<float>();
    a.state = std::make_tuple(1.0f, true);
    typename cadmium::make_message_bags<floating_accumulator<float>::input_ports>::type bags;
    cadmium::get_messages<floating_accumulator_defs::add>(bags).push_back(5.0f);
    CHECK_THROWS_AS(a.external_transition(1.0f, bags), std::logic_error);
}

TEST_CASE("pdevs accumulator output throws when not in reset state", "[pdevs][accumulator]") {
    auto a = floating_accumulator<float>();
    a.state = std::make_tuple(1.0f, false);
    CHECK_THROWS_AS(a.output(), std::logic_error);
}

TEST_CASE("pdevs accumulator output returns sum of accumulated values", "[pdevs][accumulator]") {
    auto a = floating_accumulator<float>();
    a.state = std::make_tuple(10.0f, false);

    typename cadmium::make_message_bags<floating_accumulator<float>::input_ports>::type bags;
    cadmium::get_messages<floating_accumulator_defs::add>(bags).push_back(5.0f);
    a.external_transition(10.0f, bags);
    CHECK(std::get<float>(a.state) == 15.0f);

    typename cadmium::make_message_bags<floating_accumulator<float>::input_ports>::type bags2;
    cadmium::get_messages<floating_accumulator_defs::add>(bags2).push_back(3.0f);
    cadmium::get_messages<floating_accumulator_defs::add>(bags2).push_back(7.0f);
    a.external_transition(9.0f, bags2);
    CHECK(std::get<float>(a.state) == 25.0f);

    typename cadmium::make_message_bags<floating_accumulator<float>::input_ports>::type bags3;
    cadmium::get_messages<floating_accumulator_defs::add>(bags3).push_back(3.0f);
    cadmium::get_messages<floating_accumulator_defs::reset>(bags3).emplace_back();
    a.external_transition(2.0f, bags3);
    CHECK(std::get<float>(a.state) == 28.0f);
    CHECK(std::get<bool>(a.state) == true);

    auto out = a.output();
    REQUIRE(cadmium::get_messages<floating_accumulator_defs::sum>(out).size() == 1);
    CHECK(cadmium::get_messages<floating_accumulator_defs::sum>(out)[0] == 28.0f);
}

TEST_CASE("pdevs accumulator confluence transition applies internal then external", "[pdevs][accumulator]") {
    auto a = floating_accumulator<float>();
    a.state = std::make_tuple(10.0f, false);

    // put in reset state
    typename cadmium::make_message_bags<floating_accumulator<float>::input_ports>::type bags;
    cadmium::get_messages<floating_accumulator_defs::reset>(bags).emplace_back();
    a.external_transition(0.0f, bags);
    CHECK(std::get<float>(a.state) == 10.0f);
    CHECK(std::get<bool>(a.state) == true);

    // confluence: internal (reset to 0) then external (add 2)
    typename cadmium::make_message_bags<floating_accumulator<float>::input_ports>::type bags2;
    cadmium::get_messages<floating_accumulator_defs::add>(bags2).push_back(2.0f);
    a.confluence_transition(0.0f, bags2);
    CHECK(std::get<float>(a.state) == 2.0f);
    CHECK(std::get<bool>(a.state) == false);
}
