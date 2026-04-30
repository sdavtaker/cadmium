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
#include <cadmium/basic_model/pdevs/int_generator_one_sec.hpp>
#include <cadmium/basic_model/pdevs/reset_generator_five_sec.hpp>
#include <cadmium/engine/pdevs_runner.hpp>
#include <cadmium/logger/tuple_to_ostream.hpp>
#include <cadmium/modeling/coupling.hpp>

// ---------------------------------------------------------------------------
// Shared model definitions
// ---------------------------------------------------------------------------

struct test_tick {};

using out_port = cadmium::basic_models::pdevs::generator_defs<test_tick>::out;

template <typename TIME>
struct test_generator
    : public cadmium::basic_models::pdevs::generator<test_tick, TIME> {
  float period() const override { return 1.0f; }
  test_tick output_message() const override { return test_tick{}; }
};

using iports = std::tuple<>;
struct coupled_out_port : public cadmium::out_port<test_tick> {};
using oports = std::tuple<coupled_out_port>;
using submodels = cadmium::modeling::models_tuple<test_generator>;
using eics = std::tuple<>;
using eocs = std::tuple<
    cadmium::modeling::EOC<test_generator, out_port, coupled_out_port>>;
using ics = std::tuple<>;

template <typename TIME>
using coupled_generator =
    cadmium::modeling::pdevs::coupled_model<TIME, iports, oports, submodels,
                                            eics, eocs, ics>;

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

SCENARIO("runner stops at the requested end time and returns it",
         "[pdevs][runner]") {
  GIVEN("a runner initialised at time 0 over a coupled generator model") {
    cadmium::engine::runner<float, coupled_generator> r{0.0f};
    WHEN("run_until is called with end time 60") {
      float result = r.run_until(60.0f);
      THEN("the returned time equals 60") { CHECK(result == 60.0f); }
    }
  }
}

SCENARIO("runner accepts an atomic model at the top level", "[pdevs][runner]") {
  GIVEN("a runner initialised at time 0 directly over a generator atomic") {
    cadmium::engine::runner<float, test_generator> r{0.0f};
    WHEN("run_until is called with end time 5") {
      float result = r.run_until(5.0f);
      THEN("the returned time equals 5") { CHECK(result == 5.0f); }
    }
  }
}
