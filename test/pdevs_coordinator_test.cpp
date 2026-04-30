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
#include <cadmium/concepts/pdevs_concepts.hpp>
#include <cadmium/engine/pdevs_coordinator.hpp>
#include <cadmium/engine/pdevs_engine_helpers.hpp>
#include <cadmium/engine/pdevs_simulator.hpp>
#include <cadmium/logger/tuple_to_ostream.hpp>
#include <cadmium/modeling/coupling.hpp>

// ---------------------------------------------------------------------------
// Shared model definitions
// ---------------------------------------------------------------------------

struct empty_coupled_model {
  using input_ports = std::tuple<>;
  using output_ports = std::tuple<>;
  using submodels = cadmium::modeling::models_tuple<>;
  using EICs = std::tuple<>;
  using EOCs = std::tuple<>;
  using ICs = std::tuple<>;
  template <typename TIME>
  using type =
      cadmium::modeling::pdevs::coupled_model<TIME, input_ports, output_ports,
                                              submodels, EICs, EOCs, ICs>;
};

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

template <typename TIME>
using test_accumulator = cadmium::basic_models::pdevs::accumulator<int, TIME>;
using test_accumulator_defs =
    cadmium::basic_models::pdevs::accumulator_defs<int>;

using g2a_iports = std::tuple<>;
struct g2a_coupled_out_port : public cadmium::out_port<int> {};
using g2a_oports = std::tuple<g2a_coupled_out_port>;
using g2a_submodels = cadmium::modeling::models_tuple<
    test_accumulator, cadmium::basic_models::pdevs::reset_generator_five_sec,
    cadmium::basic_models::pdevs::int_generator_one_sec>;
using g2a_eics = std::tuple<>;
using g2a_eocs = std::tuple<cadmium::modeling::EOC<
    test_accumulator, test_accumulator_defs::sum, g2a_coupled_out_port>>;
using g2a_ics = std::tuple<
    cadmium::modeling::IC<
        cadmium::basic_models::pdevs::int_generator_one_sec,
        cadmium::basic_models::pdevs::int_generator_one_sec_defs::out,
        test_accumulator, test_accumulator_defs::add>,
    cadmium::modeling::IC<
        cadmium::basic_models::pdevs::reset_generator_five_sec,
        cadmium::basic_models::pdevs::reset_generator_five_sec_defs::out,
        test_accumulator, test_accumulator_defs::reset>>;

template <typename TIME>
using coupled_g2a_model = cadmium::modeling::pdevs::coupled_model<
    TIME, g2a_iports, g2a_oports, g2a_submodels, g2a_eics, g2a_eocs, g2a_ics>;

// Two-level nesting: coupled generators + coupled accumulator under a top model

using empty_iports = std::tuple<>;
using empty_eic = std::tuple<>;
using empty_ic = std::tuple<>;

using generators_oports = std::tuple<
    cadmium::basic_models::pdevs::int_generator_one_sec_defs::out,
    cadmium::basic_models::pdevs::reset_generator_five_sec_defs::out>;
using generators_submodels = cadmium::modeling::models_tuple<
    cadmium::basic_models::pdevs::reset_generator_five_sec,
    cadmium::basic_models::pdevs::int_generator_one_sec>;
using generators_eoc = std::tuple<
    cadmium::modeling::EOC<
        cadmium::basic_models::pdevs::reset_generator_five_sec,
        cadmium::basic_models::pdevs::reset_generator_five_sec_defs::out,
        cadmium::basic_models::pdevs::reset_generator_five_sec_defs::out>,
    cadmium::modeling::EOC<
        cadmium::basic_models::pdevs::int_generator_one_sec,
        cadmium::basic_models::pdevs::int_generator_one_sec_defs::out,
        cadmium::basic_models::pdevs::int_generator_one_sec_defs::out>>;

template <typename TIME>
using coupled_generators_model = cadmium::modeling::pdevs::coupled_model<
    TIME, empty_iports, generators_oports, generators_submodels, empty_eic,
    generators_eoc, empty_ic>;

using accumulator_eic = std::tuple<
    cadmium::modeling::EIC<test_accumulator_defs::add, test_accumulator,
                           test_accumulator_defs::add>,
    cadmium::modeling::EIC<test_accumulator_defs::reset, test_accumulator,
                           test_accumulator_defs::reset>>;
using accumulator_eoc = std::tuple<cadmium::modeling::EOC<
    test_accumulator, test_accumulator_defs::sum, test_accumulator_defs::sum>>;
using accumulator_submodels = cadmium::modeling::models_tuple<test_accumulator>;

template <typename TIME>
using coupled_accumulator_model = cadmium::modeling::pdevs::coupled_model<
    TIME, typename test_accumulator<TIME>::input_ports,
    typename test_accumulator<TIME>::output_ports, accumulator_submodels,
    accumulator_eic, accumulator_eoc, empty_ic>;

using top_outport = test_accumulator_defs::sum;
using top_oports = std::tuple<top_outport>;
using top_submodels =
    cadmium::modeling::models_tuple<coupled_generators_model,
                                    coupled_accumulator_model>;
using top_eoc =
    std::tuple<cadmium::modeling::EOC<coupled_accumulator_model,
                                      test_accumulator_defs::sum, top_outport>>;
using top_ic = std::tuple<
    cadmium::modeling::IC<
        coupled_generators_model,
        cadmium::basic_models::pdevs::int_generator_one_sec_defs::out,
        coupled_accumulator_model, test_accumulator_defs::add>,
    cadmium::modeling::IC<
        coupled_generators_model,
        cadmium::basic_models::pdevs::reset_generator_five_sec_defs::out,
        coupled_accumulator_model, test_accumulator_defs::reset>>;

template <typename TIME>
using top_model = cadmium::modeling::pdevs::coupled_model<
    TIME, empty_iports, top_oports, top_submodels, empty_eic, top_eoc, top_ic>;

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

SCENARIO("an empty coupled model is not classified as atomic",
         "[pdevs][coordinator]") {
  GIVEN("the empty coupled model type") {
    WHEN("the atomic concept check is evaluated") {
      THEN("it returns false") {
        CHECK(!cadmium::concepts::pdevs::AtomicModel<
              empty_coupled_model::type<float>, float>);
      }
    }
  }
}

SCENARIO(
    "coordinator of a generator-in-coupled initialises to the generator period",
    "[pdevs][coordinator][generator]") {
  GIVEN("a coordinator wrapping a coupled model containing one generator") {
    cadmium::engine::coordinator<coupled_generator, float> cg;
    WHEN("the coordinator is initialised at time 0") {
      cg.init(0);
      THEN("next is scheduled at time 1 (the generator period)") {
        CHECK(cg.next() == 1.0f);
      }
    }
  }
}

SCENARIO("coordinator produces no output before the scheduled tick",
         "[pdevs][coordinator][generator]") {
  GIVEN("a coordinator initialised at time 0") {
    cadmium::engine::coordinator<coupled_generator, float> cg;
    cg.init(0);
    WHEN("outputs are collected at time 0.5") {
      cg.collect_outputs(0.5f);
      THEN("the outbox is empty") {
        CHECK(cadmium::get_messages<coupled_out_port>(cg.outbox()).empty());
      }
    }
  }
}

SCENARIO("coordinator rejects collecting outputs past the next scheduled time",
         "[pdevs][coordinator][generator]") {
  GIVEN("a coordinator initialised at time 0 with next scheduled at 1") {
    cadmium::engine::coordinator<coupled_generator, float> cg;
    cg.init(0);
    WHEN("collect_outputs is called at time 2 (past next)") {
      THEN("a domain_error is thrown") {
        CHECK_THROWS_AS(cg.collect_outputs(2.0f), std::domain_error);
      }
    }
  }
}

SCENARIO("coordinator delivers one tick message at the scheduled time",
         "[pdevs][coordinator][generator]") {
  GIVEN("a coordinator initialised at time 0") {
    cadmium::engine::coordinator<coupled_generator, float> cg;
    cg.init(0);
    WHEN("outputs are collected at the scheduled time of 1") {
      cg.collect_outputs(1.0f);
      auto output_bags = cg.outbox();
      THEN("the outbox contains exactly one tick message") {
        REQUIRE(!cadmium::engine::all_bags_empty(output_bags));
        CHECK(cadmium::get_messages<coupled_out_port>(output_bags).size() == 1);
      }
    }
  }
}

SCENARIO("coordinator advances correctly across two consecutive ticks",
         "[pdevs][coordinator][generator]") {
  GIVEN("a coordinator that has completed its first tick at time 1") {
    cadmium::engine::coordinator<coupled_generator, float> cg;
    cg.init(0);
    cg.collect_outputs(1.0f);
    cg.advance_simulation(1.0f);
    WHEN("the second tick cycle runs") {
      THEN("no output before time 2, a domain_error past time 2, and one "
           "message at time 2") {
        cg.collect_outputs(1.5f);
        CHECK(cadmium::get_messages<coupled_out_port>(cg.outbox()).empty());

        CHECK_THROWS_AS(cg.collect_outputs(3.0f), std::domain_error);

        cg.collect_outputs(2.0f);
        CHECK(cadmium::get_messages<coupled_out_port>(cg.outbox()).size() == 1);
      }
    }
  }
}

SCENARIO("coordinator routes generator ticks to an accumulator and outputs the "
         "count at reset",
         "[pdevs][coordinator][accumulator]") {
  GIVEN("a coordinator wrapping two generators and an accumulator that resets "
        "every 5 seconds") {
    cadmium::engine::coordinator<coupled_g2a_model, float> cc;
    cc.init(0);
    REQUIRE(cc.next() == 1.0f);
    WHEN("the simulation runs from t=1 to t=5 then the self-scheduled output "
         "is collected") {
      for (int i = 1; i <= 5; i++) {
        cc.collect_outputs(static_cast<float>(i));
        CHECK(cadmium::get_messages<g2a_coupled_out_port>(cc.outbox()).empty());
        cc.advance_simulation(static_cast<float>(i));
      }
      THEN("a self-scheduled output fires at t=5 with count 5, and the "
           "accumulator resets for t=6") {
        CHECK(cc.next() == 5.0f);
        cc.collect_outputs(5.0f);
        auto output_bags = cc.outbox();
        REQUIRE(!cadmium::engine::all_bags_empty(output_bags));
        REQUIRE(
            cadmium::get_messages<g2a_coupled_out_port>(output_bags).size() ==
            1);
        CHECK(cadmium::get_messages<g2a_coupled_out_port>(output_bags).at(0) ==
              5);

        cc.advance_simulation(5.0f);
        CHECK(cc.next() == 6.0f);

        cc.collect_outputs(6.0f);
        CHECK(cadmium::get_messages<g2a_coupled_out_port>(cc.outbox()).empty());
      }
    }
  }
}

SCENARIO("coordinator routes messages correctly through two levels of coupled "
         "models",
         "[pdevs][coordinator][nested]") {
  GIVEN("a two-level coupled model: coupled generators feeding a coupled "
        "accumulator") {
    cadmium::engine::coordinator<top_model, float> cctop;
    cctop.init(0);
    REQUIRE(cctop.next() == 1.0f);
    WHEN("the simulation runs from t=1 to t=5 then the self-scheduled output "
         "is collected") {
      for (int i = 1; i <= 5; i++) {
        cctop.collect_outputs(static_cast<float>(i));
        CHECK(cadmium::get_messages<top_outport>(cctop.outbox()).empty());
        cctop.advance_simulation(static_cast<float>(i));
      }
      THEN("the accumulated count 5 is output at t=5 and the accumulator is "
           "passive at t=6") {
        CHECK(cctop.next() == 5.0f);
        cctop.collect_outputs(5.0f);
        auto output_bags = cctop.outbox();
        REQUIRE(!cadmium::engine::all_bags_empty(output_bags));
        REQUIRE(cadmium::get_messages<top_outport>(output_bags).size() == 1);
        CHECK(cadmium::get_messages<top_outport>(output_bags).at(0) == 5);

        cctop.advance_simulation(5.0f);
        CHECK(cctop.next() == 6.0f);
        cctop.collect_outputs(6.0f);
        CHECK(cadmium::get_messages<top_outport>(cctop.outbox()).empty());
      }
    }
  }
}
