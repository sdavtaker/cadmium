/**
 * Copyright (c) 2017-present, Damian Vicino, Laouen M. L. Belloli
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

#include <any>
#include <catch2/catch_test_macros.hpp>
#include <limits>

#include <cadmium/basic_model/pdevs/accumulator.hpp>
#include <cadmium/engine/pdevs_dynamic_simulator.hpp>
#include <cadmium/logger/tuple_to_ostream.hpp>
#include <cadmium/modeling/dynamic_atomic.hpp>
#include <cadmium/modeling/dynamic_message_bag.hpp>
#include <cadmium/modeling/dynamic_model_translator.hpp>

template <typename TIME>
using int_accumulator = cadmium::basic_models::pdevs::accumulator<int, TIME>;
using int_accumulator_defs =
    cadmium::basic_models::pdevs::accumulator_defs<int>;

using input_ports = int_accumulator<float>::input_ports;
using in_bags_type = typename cadmium::make_message_bags<input_ports>::type;

static auto make_sim() {
  auto model =
      cadmium::dynamic::translate::make_dynamic_atomic_model<int_accumulator,
                                                             float>();
  return cadmium::dynamic::engine::simulator<float>(model);
}

static cadmium::dynamic::message_bags
make_input(std::initializer_list<int> add_vals, bool reset) {
  cadmium::message_bag<int_accumulator_defs::add> bag_add;
  cadmium::message_bag<int_accumulator_defs::reset> bag_reset;
  bag_add.messages.assign(add_vals);
  if (reset)
    bag_reset.messages.emplace_back();
  cadmium::dynamic::message_bags bags;
  bags[typeid(int_accumulator_defs::add)] = bag_add;
  bags[typeid(int_accumulator_defs::reset)] = bag_reset;
  return bags;
}

SCENARIO("dynamic accumulator simulator starts in passive state",
         "[dynamic][simulator]") {
  GIVEN("a dynamic accumulator simulator initialised at time 0") {
    auto s = make_sim();
    s.init(0.0f);
    WHEN("next is queried") {
      THEN("it is infinity") {
        CHECK(s.next() == std::numeric_limits<float>::infinity());
      }
    }
  }
}

SCENARIO(
    "dynamic accumulator simulator receiving only add messages stays passive",
    "[dynamic][simulator]") {
  GIVEN("a dynamic accumulator simulator initialised at time 0") {
    auto s = make_sim();
    s.init(0.0f);
    WHEN("add messages are delivered and simulation is advanced to time 3") {
      s._inbox = make_input({1, 2, 3, 4}, false);
      s.advance_simulation(3.0f);
      THEN("next remains infinity because no reset was received") {
        CHECK(s.next() == std::numeric_limits<float>::infinity());
      }
    }
  }
}

SCENARIO("dynamic accumulator simulator schedules internal transition when "
         "reset is received",
         "[dynamic][simulator]") {
  GIVEN("a dynamic accumulator that has received add messages and is then "
        "reset at time 4") {
    auto s = make_sim();
    s.init(0.0f);
    s._inbox = make_input({1, 2, 3, 4}, false);
    s.advance_simulation(3.0f);
    WHEN("a reset message arrives at time 4") {
      s._inbox = make_input({}, true);
      s.advance_simulation(4.0f);
      THEN("next is scheduled at time 4") { CHECK(s.next() == 4.0f); }
    }
  }
}

SCENARIO("dynamic accumulator simulator output is the sum of all received add "
         "messages",
         "[dynamic][simulator]") {
  GIVEN("a dynamic accumulator that accumulated 1+2+3+4 and was then reset at "
        "time 4") {
    auto s = make_sim();
    s.init(0.0f);
    s._inbox = make_input({1, 2, 3, 4}, false);
    s.advance_simulation(3.0f);
    s._inbox = make_input({}, true);
    s.advance_simulation(4.0f);
    WHEN("outputs are collected at time 4") {
      s.collect_outputs(4.0f);
      auto o = s.outbox();
      auto out = std::any_cast<cadmium::message_bag<int_accumulator_defs::sum>>(
          o.at(typeid(int_accumulator_defs::sum)));
      THEN("the sum port contains exactly 10") {
        REQUIRE(out.messages.size() == 1);
        CHECK(out.messages.at(0) == 10);
      }
    }
  }
}

SCENARIO("dynamic accumulator simulator internal transition clears the counter "
         "before the next reset",
         "[dynamic][simulator]") {
  GIVEN("a dynamic accumulator reset at time 4 with no prior add messages, "
        "then drained") {
    auto s = make_sim();
    s.init(0.0f);
    s._inbox = make_input({}, true);
    s.advance_simulation(4.0f);
    s._inbox =
        cadmium::dynamic::modeling::create_empty_message_bags<in_bags_type>();
    s.advance_simulation(4.0f);
    REQUIRE(s.next() == std::numeric_limits<float>::infinity());
    WHEN("a second reset arrives at time 5 with no add messages") {
      s._inbox = make_input({}, true);
      s.advance_simulation(5.0f);
      REQUIRE(s.next() == 5.0f);
      s.collect_outputs(5.0f);
      auto o = s.outbox();
      auto out = std::any_cast<cadmium::message_bag<int_accumulator_defs::sum>>(
          o.at(typeid(int_accumulator_defs::sum)));
      THEN("the output sum is 0") {
        REQUIRE(out.messages.size() == 1);
        CHECK(out.messages.at(0) == 0);
      }
    }
  }
}
