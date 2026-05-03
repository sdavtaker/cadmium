/**
 * Copyright (c) 2017-present, Laouen M. L. Belloli
 * Carleton University, Universidad de Buenos Aires
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

#include <cadmium/basic_model/pdevs/accumulator.hpp>
#include <cadmium/modeling/dynamic_atomic.hpp>
#include <cadmium/modeling/dynamic_model_translator.hpp>

#include <catch2/catch_test_macros.hpp>

template <typename TIME>
using int_accumulator = cadmium::basic_models::pdevs::accumulator<int, TIME>;

template <typename TIME>
struct test_custom_accumulator : public cadmium::basic_models::pdevs::accumulator<int, TIME> {
    test_custom_accumulator() = default;
    explicit test_custom_accumulator(int) {}
};

SCENARIO("dynamic atomic wrapper preserves the model state type", "[dynamic][atomic]") {
    GIVEN("a raw int_accumulator and a dynamic atomic wrapping the same model "
          "type") {
        int_accumulator<float> model;
        cadmium::dynamic::modeling::atomic<int_accumulator, float> wrapped_model;
        static_assert(std::is_same<decltype(model.state), decltype(wrapped_model.state)>::value);
        WHEN("their initial states are compared") {
            THEN("the states are equal") {
                CHECK(model.state == wrapped_model.state);
            }
        }
    }
}

SCENARIO("dynamic atomic can be constructed with a custom id and constructor "
         "arguments",
         "[dynamic][atomic]") {
    GIVEN("a model type whose constructor takes an int") {
        WHEN("make_dynamic_atomic_model is called with a custom id and an int "
             "argument") {
            THEN("no exception is thrown") {
                CHECK_NOTHROW(cadmium::dynamic::translate::make_dynamic_atomic_model<
                              test_custom_accumulator, float, int>("id_test", 2));
            }
        }
    }
}
