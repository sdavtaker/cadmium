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
#include <limits>

#include <cadmium/basic_model/pdevs/accumulator.hpp>
#include <cadmium/basic_model/pdevs/generator.hpp>
#include <cadmium/engine/pdevs_simulator.hpp>
#include <cadmium/logger/tuple_to_ostream.hpp>

template<typename TIME>
using int_accumulator = cadmium::basic_models::pdevs::accumulator<int, TIME>;
using int_accumulator_defs = cadmium::basic_models::pdevs::accumulator_defs<int>;

using sim_t = cadmium::engine::simulator<int_accumulator, float, cadmium::logger::not_logger>;
using input_ports = int_accumulator<float>::input_ports;
using input_bags_t = typename cadmium::make_message_bags<input_ports>::type;

TEST_CASE("accumulator simulator initializes passive", "[pdevs][simulator][accumulator]") {
    sim_t s;
    s.init(0.0f);
    CHECK(s.next() == std::numeric_limits<float>::infinity());
}

TEST_CASE("accumulator simulator external transition on add port", "[pdevs][simulator][accumulator]") {
    sim_t s;
    s.init(0.0f);

    input_bags_t bags{};
    cadmium::get_messages<int_accumulator_defs::add>(bags).assign({1, 2, 3, 4});

    s.inbox(bags);
    s.advance_simulation(3.0f);

    CHECK(s.next() == std::numeric_limits<float>::infinity());
}

TEST_CASE("accumulator simulator reset port triggers internal transition", "[pdevs][simulator][accumulator]") {
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

    CHECK(s.next() == 4.0f);
}

TEST_CASE("accumulator simulator output is sum of add messages", "[pdevs][simulator][accumulator]") {
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

    s.collect_outputs(4.0f);
    auto o = s.outbox();

    REQUIRE(!cadmium::engine::all_bags_empty(o));
    REQUIRE(cadmium::get_messages<int_accumulator_defs::sum>(o).size() == 1);
    CHECK(cadmium::get_messages<int_accumulator_defs::sum>(o).at(0) == 10);
}

TEST_CASE("accumulator simulator internal transition clears counter", "[pdevs][simulator][accumulator]") {
    sim_t s;
    s.init(0.0f);

    input_bags_t reset_bags{};
    cadmium::get_messages<int_accumulator_defs::reset>(reset_bags).emplace_back();
    s.inbox(reset_bags);
    s.advance_simulation(3.0f);

    // drain internal transition
    input_bags_t empty{};
    s.inbox(empty);
    s.advance_simulation(3.0f);
    CHECK(s.next() == std::numeric_limits<float>::infinity());

    // add values then reset again
    input_bags_t bags{};
    cadmium::get_messages<int_accumulator_defs::reset>(bags).emplace_back();
    s.inbox(bags);
    s.advance_simulation(5.0f);

    s.collect_outputs(5.0f);
    auto o = s.outbox();
    REQUIRE(cadmium::get_messages<int_accumulator_defs::sum>(o).size() == 1);
    CHECK(cadmium::get_messages<int_accumulator_defs::sum>(o).at(0) == 0);
}

TEST_CASE("accumulator simulator throws on past advance", "[pdevs][simulator][accumulator]") {
    sim_t s;
    s.init(0.0f);

    input_bags_t bags{};
    cadmium::get_messages<int_accumulator_defs::add>(bags).assign({1, 2, 3, 4});
    cadmium::get_messages<int_accumulator_defs::reset>(bags).emplace_back();
    s.inbox(bags);
    s.advance_simulation(3.0f);

    s.inbox(bags);
    CHECK_THROWS_AS(s.advance_simulation(2.0f), std::domain_error);
}

TEST_CASE("accumulator simulator throws on advance past next internal event", "[pdevs][simulator][accumulator]") {
    sim_t s;
    s.init(0.0f);

    input_bags_t bags{};
    cadmium::get_messages<int_accumulator_defs::add>(bags).assign({1, 2, 3, 4});
    cadmium::get_messages<int_accumulator_defs::reset>(bags).emplace_back();
    s.inbox(bags);
    s.advance_simulation(3.0f);

    s.inbox(bags);
    CHECK_THROWS_AS(s.advance_simulation(4.0f), std::domain_error);
}

// Generator simulator tests

const float init_period = 1.0f;
const float init_output_message = 2.0f;

template<typename TIME>
using floating_generator_base = cadmium::basic_models::pdevs::generator<float, TIME>;
using floating_generator_defs = cadmium::basic_models::pdevs::generator_defs<float>;

template<typename TIME>
struct floating_generator : public floating_generator_base<TIME> {
    float period() const override { return init_period; }
    float output_message() const override { return init_output_message; }
};

using gen_sim_t = cadmium::engine::simulator<floating_generator, float, cadmium::logger::not_logger>;

TEST_CASE("generator simulator schedules first output at period after init", "[pdevs][simulator][generator]") {
    gen_sim_t s;
    s.init(0.0f);
    CHECK(s.next() == 1.0f);
}

TEST_CASE("generator simulator produces no output before scheduled time", "[pdevs][simulator][generator]") {
    gen_sim_t s;
    s.init(0.0f);
    s.collect_outputs(0.5f);
    CHECK(cadmium::engine::all_bags_empty(s.outbox()));
}

TEST_CASE("generator simulator produces correct output at scheduled time", "[pdevs][simulator][generator]") {
    gen_sim_t s;
    s.init(0.0f);
    s.collect_outputs(1.0f);
    auto out = s.outbox();
    REQUIRE(cadmium::get_messages<floating_generator_defs::out>(out).size() == 1);
    CHECK(cadmium::get_messages<floating_generator_defs::out>(out)[0] == 2.0f);
}

TEST_CASE("generator simulator reschedules next output after advance", "[pdevs][simulator][generator]") {
    gen_sim_t s;
    s.init(0.0f);
    s.advance_simulation(1.0f);
    CHECK(s.next() == 2.0f);
}
