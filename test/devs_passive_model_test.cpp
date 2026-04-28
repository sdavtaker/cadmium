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

#include <cadmium/basic_model/devs/passive.hpp>
#include <cadmium/concept/concept_helpers.hpp>

template<typename TIME>
using floating_passive = cadmium::basic_models::devs::passive<float, TIME>;
using floating_passive_defs = cadmium::basic_models::devs::passive_defs<float>;

TEST_CASE("devs passive is atomic", "[devs][passive]") {
    CHECK(cadmium::model_checks::is_atomic<floating_passive>::value());
}

TEST_CASE("devs passive is constructable", "[devs][passive]") {
    CHECK_NOTHROW(floating_passive<float>{});
}

TEST_CASE("devs passive throws on internal transition", "[devs][passive]") {
    auto p = floating_passive<float>();
    CHECK_THROWS_AS(p.internal_transition(), std::logic_error);
}

TEST_CASE("devs passive throws on output", "[devs][passive]") {
    auto p = floating_passive<float>();
    CHECK_THROWS_AS(p.output(), std::logic_error);
}

TEST_CASE("devs passive external transition preserves infinite time advance", "[devs][passive]") {
    auto p = floating_passive<float>();
    REQUIRE(std::isinf(p.time_advance()));
    typename cadmium::make_message_box<floating_passive<float>::input_ports>::type input;
    cadmium::get_message<floating_passive_defs::in>(input).emplace(1);
    CHECK_NOTHROW(p.external_transition(5.0, input));
    CHECK(std::isinf(p.time_advance()));
}
