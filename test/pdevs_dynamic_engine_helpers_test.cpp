/**
 * Copyright (c) 2017-present, Laouen M. L. Belloli, Damian Vicino
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

#include <cadmium/engine/pdevs_dynamic_engine_helpers.hpp>
#include <cadmium/modeling/dynamic_message_bag.hpp>
#include <cadmium/modeling/dynamic_models_helpers.hpp>
#include <cadmium/modeling/message_bag.hpp>
#include <cadmium/modeling/ports.hpp>

#include <catch2/catch_test_macros.hpp>
#include <typeindex>

// Port types must be at namespace scope — local structs cannot be template
// arguments in C++
namespace {
    struct eh_test_in_0 : public cadmium::in_port<int> {};
    struct eh_test_in_1 : public cadmium::in_port<double> {};
    using eh_test_input_ports = std::tuple<eh_test_in_0, eh_test_in_1>;
    using eh_input_bags       = typename cadmium::make_message_bags<eh_test_input_ports>::type;
} // namespace

SCENARIO("all_bags_empty reports true when no messages are present", "[dynamic][engine_helpers]") {
    GIVEN("a dynamic message bag created from empty typed bags") {
        auto empty_box = cadmium::dynamic::modeling::create_empty_message_bags<eh_input_bags>();
        WHEN("all_bags_empty is evaluated") {
            THEN("it returns true") {
                CHECK(cadmium::dynamic::engine::all_bags_empty<eh_input_bags>(empty_box));
            }
        }
    }
}

SCENARIO("all_bags_empty reports false when messages are present", "[dynamic][engine_helpers]") {
    GIVEN("a dynamic message bag containing int and double messages") {
        eh_input_bags bs_tuple;
        cadmium::get_messages<eh_test_in_0>(bs_tuple) = {1, 2};
        cadmium::get_messages<eh_test_in_1>(bs_tuple) = {1.5, 2.5};
        cadmium::dynamic::message_bags bs_map;
        cadmium::dynamic::modeling::fill_map_from_bags(bs_tuple, bs_map);
        WHEN("all_bags_empty is evaluated") {
            THEN("it returns false") {
                CHECK(!cadmium::dynamic::engine::all_bags_empty<eh_input_bags>(bs_map));
            }
        }
    }
}
