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

#include <cadmium/basic_model/pdevs/accumulator.hpp>
#include <cadmium/basic_model/pdevs/generator.hpp>
#include <cadmium/engine/pdevs_coordinator.hpp>
#include <cadmium/engine/pdevs_engine_helpers.hpp>
#include <cadmium/engine/pdevs_simulator.hpp>

template <typename TIME>
using floating_accumulator =
    cadmium::basic_models::pdevs::accumulator<float, TIME>;

using simulator_of_floating_accumulator =
    cadmium::engine::simulator<floating_accumulator, float>;
using tuple_sim_accum = std::tuple<simulator_of_floating_accumulator>;

SCENARIO(
    "get_engine_by_model retrieves the single engine from a one-element tuple",
    "[pdevs][engine_helpers]") {
  GIVEN("a tuple containing one accumulator simulator") {
    tuple_sim_accum st;
    WHEN("get_engine_by_model is called for the accumulator type") {
      THEN("no exception is thrown") {
        CHECK_NOTHROW(
            cadmium::engine::get_engine_by_model<floating_accumulator<float>,
                                                 tuple_sim_accum>(st));
      }
    }
  }
}

const float gen_period = 0.1f;
const float gen_output = 1.0f;

template <typename TIME>
using floating_generator_base =
    cadmium::basic_models::pdevs::generator<float, TIME>;

template <typename TIME>
struct floating_generator_a : public floating_generator_base<TIME> {
  float period() const override { return gen_period; }
  float output_message() const override { return gen_output; }
};

template <typename TIME>
struct floating_generator_b : public floating_generator_base<TIME> {
  float period() const override { return gen_period; }
  float output_message() const override { return gen_output; }
};

using simulator_of_gen_a =
    cadmium::engine::simulator<floating_generator_a, float>;
using simulator_of_gen_b =
    cadmium::engine::simulator<floating_generator_b, float>;
using tuple_sim_gens = std::tuple<simulator_of_gen_a, simulator_of_gen_b>;

SCENARIO(
    "get_engine_by_model retrieves the first engine from a two-element tuple",
    "[pdevs][engine_helpers]") {
  GIVEN("a tuple containing two generator simulators") {
    tuple_sim_gens st;
    WHEN("get_engine_by_model is called for the first generator type") {
      THEN("no exception is thrown") {
        CHECK_NOTHROW(
            cadmium::engine::get_engine_by_model<floating_generator_a<float>,
                                                 tuple_sim_gens>(st));
      }
    }
  }
}

SCENARIO(
    "get_engine_by_model retrieves the second engine from a two-element tuple",
    "[pdevs][engine_helpers]") {
  GIVEN("a tuple containing two generator simulators") {
    tuple_sim_gens st;
    WHEN("get_engine_by_model is called for the second generator type") {
      THEN("no exception is thrown") {
        CHECK_NOTHROW(
            cadmium::engine::get_engine_by_model<floating_generator_b<float>,
                                                 tuple_sim_gens>(st));
      }
    }
  }
}
