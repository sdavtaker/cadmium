/**
 * Copyright (c) 2018-present, Laouen M. L. Belloli, Damian Vicino
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
#include <memory>
#include <tuple>
#include <typeindex>

#include <cadmium/basic_model/pdevs/generator.hpp>
#include <cadmium/engine/pdevs_dynamic_coordinator.hpp>
#include <cadmium/modeling/coupling.hpp>
#include <cadmium/modeling/dynamic_atomic.hpp>
#include <cadmium/modeling/dynamic_coupled.hpp>
#include <cadmium/modeling/dynamic_model_translator.hpp>
#include <cadmium/modeling/ports.hpp>

template <typename TIME>
class custom_id_coupled : public cadmium::dynamic::modeling::coupled<TIME> {
public:
  custom_id_coupled()
      : cadmium::dynamic::modeling::coupled<TIME>("custom_id_coupled") {}
};

SCENARIO("dynamic coordinator model ID is taken from the coupled model",
         "[dynamic][coordinator]") {
  GIVEN("a coupled model with id 'custom_id_coupled'") {
    auto coupled = std::make_shared<custom_id_coupled<float>>();
    cadmium::dynamic::engine::coordinator<float> c(coupled);
    WHEN("the coordinator model ID is queried") {
      THEN("it matches the coupled model ID") {
        CHECK(coupled->get_id() == "custom_id_coupled");
        CHECK(coupled->get_id() == c.get_model_id());
      }
    }
  }
}

namespace {
struct test_tick {};

using out_port = cadmium::basic_models::pdevs::generator_defs<test_tick>::out;

template <typename TIME>
using test_tick_generator_base =
    cadmium::basic_models::pdevs::generator<test_tick, float>;

template <typename TIME>
struct test_generator : public test_tick_generator_base<TIME> {
  float period() const override { return 1.0f; }
  test_tick output_message() const override { return test_tick{}; }
};

template <typename TIME>
using dynamic_test_generator =
    cadmium::dynamic::modeling::atomic<test_generator, TIME>;

struct coupled_out_port : public cadmium::out_port<test_tick> {};

using iports = std::tuple<>;
using oports = std::tuple<coupled_out_port>;
using eocs = std::tuple<
    cadmium::modeling::EOC<dynamic_test_generator, out_port, coupled_out_port>>;
using eics = std::tuple<>;
using ics = std::tuple<>;

static std::shared_ptr<cadmium::dynamic::modeling::coupled<float>>
make_generator_coupled() {
  auto sp_gen =
      cadmium::dynamic::translate::make_dynamic_atomic_model<test_generator,
                                                             float>();

  cadmium::dynamic::translate::models_by_type mbt;
  mbt.emplace(typeid(dynamic_test_generator<float>), sp_gen);

  auto input_ports = cadmium::dynamic::translate::make_ports<iports>();
  auto output_ports = cadmium::dynamic::translate::make_ports<oports>();
  cadmium::dynamic::modeling::Models submodels = {sp_gen};
  auto dynamic_eocs =
      cadmium::dynamic::translate::make_dynamic_eoc<float, eocs>(mbt);
  auto dynamic_eics =
      cadmium::dynamic::translate::make_dynamic_eic<float, eics>(mbt);
  auto dynamic_ics =
      cadmium::dynamic::translate::make_dynamic_ic<float, ics>(mbt);

  return std::make_shared<cadmium::dynamic::modeling::coupled<float>>(
      "dynamic_coupled_test_generator", submodels, input_ports, output_ports,
      dynamic_eics, dynamic_eocs, dynamic_ics);
}
} // namespace

SCENARIO("dynamic coordinator of a generator-in-coupled initialises to the "
         "generator period",
         "[dynamic][coordinator]") {
  GIVEN("a dynamic coordinator wrapping a generator-in-coupled model") {
    auto coupled = make_generator_coupled();
    cadmium::dynamic::engine::coordinator<float> cg(coupled);
    WHEN("the coordinator is initialised at time 0") {
      cg.init(0);
      THEN("next is scheduled at time 1") { CHECK(cg.next() == 1.0f); }
    }
  }
}

SCENARIO("dynamic coordinator produces no output before the scheduled tick",
         "[dynamic][coordinator]") {
  GIVEN("a dynamic coordinator initialised at time 0") {
    auto coupled = make_generator_coupled();
    cadmium::dynamic::engine::coordinator<float> cg(coupled);
    cg.init(0);
    WHEN(
        "outputs are collected at time 0.5 (before the first scheduled tick)") {
      cg.collect_outputs(0.5f);
      THEN("the outbox is empty") { CHECK(cg.outbox().empty()); }
    }
  }
}

SCENARIO("dynamic coordinator rejects collecting outputs past the next "
         "scheduled time",
         "[dynamic][coordinator]") {
  GIVEN(
      "a dynamic coordinator initialised at time 0 with next scheduled at 1") {
    auto coupled = make_generator_coupled();
    cadmium::dynamic::engine::coordinator<float> cg(coupled);
    cg.init(0);
    WHEN("collect_outputs is called at time 2 (past next)") {
      THEN("a domain_error is thrown") {
        CHECK_THROWS_AS(cg.collect_outputs(2.0f), std::domain_error);
      }
    }
  }
}

SCENARIO("dynamic coordinator delivers one tick message at the scheduled time",
         "[dynamic][coordinator]") {
  GIVEN("a dynamic coordinator initialised at time 0") {
    auto coupled = make_generator_coupled();
    cadmium::dynamic::engine::coordinator<float> cg(coupled);
    cg.init(0);
    WHEN("outputs are collected at the scheduled time of 1") {
      cg.collect_outputs(1.0f);
      auto bags = cg.outbox();
      THEN("the outbox contains exactly one message on the expected output "
           "port") {
        REQUIRE(!bags.empty());
        REQUIRE(bags.size() == 1);
        REQUIRE(bags.find(typeid(coupled_out_port)) != bags.end());
        CHECK(std::any_cast<cadmium::message_bag<coupled_out_port>>(
                  bags.at(typeid(coupled_out_port)))
                  .messages.size() == 1);
      }
    }
  }
}

SCENARIO("dynamic coordinator advances correctly across two consecutive ticks",
         "[dynamic][coordinator]") {
  GIVEN("a dynamic coordinator that has completed its first tick at time 1") {
    auto coupled = make_generator_coupled();
    cadmium::dynamic::engine::coordinator<float> cg(coupled);
    cg.init(0);
    cg.collect_outputs(1.0f);
    cg.advance_simulation(1.0f);
    WHEN("the second tick cycle runs") {
      THEN("no output before time 2, a domain_error past time 2, and one "
           "message at time 2") {
        cg.collect_outputs(1.5f);
        CHECK(cg.outbox().empty());

        CHECK_THROWS_AS(cg.collect_outputs(3.0f), std::domain_error);

        cg.collect_outputs(2.0f);
        auto bags = cg.outbox();
        REQUIRE(!bags.empty());
        REQUIRE(bags.size() == 1);
        REQUIRE(bags.find(typeid(coupled_out_port)) != bags.end());
        CHECK(std::any_cast<cadmium::message_bag<coupled_out_port>>(
                  bags.at(typeid(coupled_out_port)))
                  .messages.size() == 1);
      }
    }
  }
}
