/**
 * Copyright (c) 2018-present, Laouen M. L. Belloli, Damian Vicino
 * Carleton University, Universite de Nice-Sophia Antipolis, Universidad de
 * Buenos Aires All rights reserved.
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

#include <cadmium/basic_model/pdevs/generator.hpp>
#include <cadmium/engine/pdevs_dynamic_runner.hpp>
#include <cadmium/modeling/coupling.hpp>
#include <cadmium/modeling/dynamic_atomic.hpp>
#include <cadmium/modeling/dynamic_coupled.hpp>
#include <cadmium/modeling/dynamic_model_translator.hpp>

#include <catch2/catch_test_macros.hpp>

namespace {
    struct test_tick {};

    using out_port = cadmium::basic_models::pdevs::generator_defs<test_tick>::out;

    template <typename TIME>
    using test_tick_generator_base = cadmium::basic_models::pdevs::generator<test_tick, TIME>;

    template <typename TIME> struct test_generator : public test_tick_generator_base<TIME> {
        float period() const override {
            return 1.0f;
        }
        test_tick output_message() const override {
            return test_tick{};
        }
    };

    struct coupled_out_port : public cadmium::out_port<test_tick> {};

    using iports    = std::tuple<>;
    using oports    = std::tuple<coupled_out_port>;
    using submodels = cadmium::modeling::models_tuple<test_generator>;
    using eics      = std::tuple<>;
    using eocs = std::tuple<cadmium::modeling::EOC<test_generator, out_port, coupled_out_port>>;
    using ics  = std::tuple<>;

    template <typename TIME>
    using coupled_generator =
        cadmium::modeling::pdevs::coupled_model<TIME, iports, oports, submodels, eics, eocs, ics>;

    auto coupled =
        cadmium::dynamic::translate::make_dynamic_coupled_model<float, coupled_generator>();

    auto atomic = cadmium::dynamic::translate::make_dynamic_atomic_model<test_generator, float>();
} // namespace

SCENARIO("dynamic runner stops at the requested end time and returns it", "[dynamic][runner]") {
    GIVEN("a dynamic runner initialised at time 0 over a coupled generator") {
        cadmium::dynamic::engine::runner<float> r(coupled, 0.0);
        WHEN("run_until is called with end time 60") {
            float end = r.run_until(60.0);
            THEN("the returned time equals 60") {
                CHECK(end == 60.0f);
            }
        }
    }
}

SCENARIO("dynamic runner accepts an atomic model at the top level", "[dynamic][runner]") {
    GIVEN("a dynamic runner initialised at time 0 directly over an atomic") {
        cadmium::dynamic::engine::runner<float> r(atomic, 0.0);
        WHEN("run_until is called with end time 5") {
            float end = r.run_until(5.0);
            THEN("the returned time equals 5") {
                CHECK(end == 5.0f);
            }
        }
    }
}
