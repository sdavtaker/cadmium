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

#include <any>
#include <catch2/catch_test_macros.hpp>
#include <map>
#include <typeindex>

#include <cadmium/modeling/dynamic_models_helpers.hpp>
#include <cadmium/modeling/message_bag.hpp>
#include <cadmium/modeling/ports.hpp>

// Port types must be at namespace scope — local structs cannot be template
// arguments in C++
namespace {
struct ah_test_in_0 : public cadmium::in_port<int> {};
struct ah_test_in_1 : public cadmium::in_port<double> {};
using ah_test_input_ports = std::tuple<ah_test_in_0, ah_test_in_1>;
using ah_input_bags =
    typename cadmium::make_message_bags<ah_test_input_ports>::type;
} // namespace

SCENARIO("fill_bags_from_map moves messages from a type-indexed map into a "
         "typed bag tuple",
         "[dynamic][helpers]") {
  GIVEN("a type-indexed map with two bags containing int and double messages") {
    cadmium::message_bag<ah_test_in_0> bag_0;
    cadmium::message_bag<ah_test_in_1> bag_1;
    bag_0.messages = {1, 2};
    bag_1.messages = {1.5, 2.5};
    std::map<std::type_index, std::any> bs_map;
    bs_map[typeid(ah_test_in_0)] = bag_0;
    bs_map[typeid(ah_test_in_1)] = bag_1;
    WHEN("fill_bags_from_map is called") {
      ah_input_bags bs_tuple;
      cadmium::dynamic::modeling::fill_bags_from_map<ah_input_bags>(bs_map,
                                                                    bs_tuple);
      THEN("each typed bag in the tuple has the same message count as the "
           "source") {
        CHECK(cadmium::get_messages<ah_test_in_0>(bs_tuple).size() ==
              bag_0.messages.size());
        CHECK(cadmium::get_messages<ah_test_in_1>(bs_tuple).size() ==
              bag_1.messages.size());
      }
    }
  }
}

SCENARIO("fill_map_from_bags moves messages from a typed bag tuple into a "
         "type-indexed map",
         "[dynamic][helpers]") {
  GIVEN("a typed bag tuple with int and double messages") {
    ah_input_bags bs_tuple;
    cadmium::get_messages<ah_test_in_0>(bs_tuple) = {1, 2};
    cadmium::get_messages<ah_test_in_1>(bs_tuple) = {1.5, 2.5};
    WHEN("fill_map_from_bags is called") {
      std::map<std::type_index, std::any> bs_map;
      cadmium::dynamic::modeling::fill_map_from_bags<ah_input_bags>(bs_tuple,
                                                                    bs_map);
      THEN("each entry in the map contains two messages") {
        auto map_bag_0 = std::any_cast<cadmium::message_bag<ah_test_in_0>>(
            bs_map.at(typeid(ah_test_in_0)));
        auto map_bag_1 = std::any_cast<cadmium::message_bag<ah_test_in_1>>(
            bs_map.at(typeid(ah_test_in_1)));
        CHECK(map_bag_0.messages.size() == 2);
        CHECK(map_bag_1.messages.size() == 2);
      }
    }
  }
}
