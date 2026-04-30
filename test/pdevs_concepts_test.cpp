/**
 * Copyright (c) 2013-present, Damian Vicino, Laouen M. L. Belloli
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

/**
 * Concept-satisfaction tests for the PDEVS modeling language.
 *
 * Replaces the old compile-fail / compile-success harness under test-compile/.
 * Every check is a STATIC_REQUIRE so failures are caught at compile time and
 * reported as normal test output rather than as a broken build.
 */

#include <catch2/catch_test_macros.hpp>
#include <tuple>

#include <cadmium/basic_model/devs/generator.hpp>
#include <cadmium/basic_model/devs/passive.hpp>
#include <cadmium/basic_model/pdevs/accumulator.hpp>
#include <cadmium/basic_model/pdevs/generator.hpp>
#include <cadmium/basic_model/pdevs/passive.hpp>
#include <cadmium/concepts/pdevs_concepts.hpp>
#include <cadmium/modeling/coupling.hpp>
#include <cadmium/modeling/message_bag.hpp>
#include <cadmium/modeling/message_box.hpp>
#include <cadmium/modeling/ports.hpp>

namespace {

using cadmium::concepts::pdevs::AtomicModel;
using cadmium::concepts::pdevs::CoupledModel;
namespace devs_concepts = cadmium::concepts::devs;

// ═══════════════════════════════════════════════════════════════════════════
// Valid atomic models
// ═══════════════════════════════════════════════════════════════════════════

template <typename TIME> struct valid_atomic {
  struct in : public cadmium::in_port<int> {};
  struct out : public cadmium::out_port<int> {};
  using state_type = int;
  state_type state = 0;
  using input_ports = std::tuple<in>;
  using output_ports = std::tuple<out>;
  void internal_transition() {}
  void
  external_transition(TIME,
                      typename cadmium::make_message_bags<input_ports>::type) {}
  void confluence_transition(
      TIME, typename cadmium::make_message_bags<input_ports>::type) {}
  typename cadmium::make_message_bags<output_ports>::type output() const {
    return {};
  }
  TIME time_advance() const { return TIME{}; }
};

template <typename TIME> struct valid_atomic_multi_port {
  struct in1 : public cadmium::in_port<int> {};
  struct in2 : public cadmium::in_port<float> {};
  struct in3 : public cadmium::in_port<double> {};
  struct out1 : public cadmium::out_port<int> {};
  struct out2 : public cadmium::out_port<float> {};
  using state_type = int;
  state_type state = 0;
  using input_ports = std::tuple<in1, in2, in3>;
  using output_ports = std::tuple<out1, out2>;
  void internal_transition() {}
  void
  external_transition(TIME,
                      typename cadmium::make_message_bags<input_ports>::type) {}
  void confluence_transition(
      TIME, typename cadmium::make_message_bags<input_ports>::type) {}
  typename cadmium::make_message_bags<output_ports>::type output() const {
    return {};
  }
  TIME time_advance() const { return TIME{}; }
};

// ═══════════════════════════════════════════════════════════════════════════
// Invalid atomic models — one violation each
// ═══════════════════════════════════════════════════════════════════════════

template <typename TIME> struct atomic_no_state_type {
  struct in : public cadmium::in_port<int> {};
  struct out : public cadmium::out_port<int> {};
  int state = 0; // state variable present but state_type typedef missing
  using input_ports = std::tuple<in>;
  using output_ports = std::tuple<out>;
  void internal_transition() {}
  void
  external_transition(TIME,
                      typename cadmium::make_message_bags<input_ports>::type) {}
  void confluence_transition(
      TIME, typename cadmium::make_message_bags<input_ports>::type) {}
  typename cadmium::make_message_bags<output_ports>::type output() const {
    return {};
  }
  TIME time_advance() const { return TIME{}; }
};

template <typename TIME> struct atomic_no_state_var {
  struct in : public cadmium::in_port<int> {};
  struct out : public cadmium::out_port<int> {};
  using state_type = int;
  // state variable missing
  using input_ports = std::tuple<in>;
  using output_ports = std::tuple<out>;
  void internal_transition() {}
  void
  external_transition(TIME,
                      typename cadmium::make_message_bags<input_ports>::type) {}
  void confluence_transition(
      TIME, typename cadmium::make_message_bags<input_ports>::type) {}
  typename cadmium::make_message_bags<output_ports>::type output() const {
    return {};
  }
  TIME time_advance() const { return TIME{}; }
};

template <typename TIME> struct atomic_no_input_ports {
  struct out : public cadmium::out_port<int> {};
  using state_type = int;
  state_type state = 0;
  // input_ports missing
  using output_ports = std::tuple<out>;
  void internal_transition() {}
  typename cadmium::make_message_bags<output_ports>::type output() const {
    return {};
  }
  TIME time_advance() const { return TIME{}; }
};

template <typename TIME> struct atomic_no_output_ports {
  struct in : public cadmium::in_port<int> {};
  using state_type = int;
  state_type state = 0;
  using input_ports = std::tuple<in>;
  // output_ports missing
  void internal_transition() {}
  void
  external_transition(TIME,
                      typename cadmium::make_message_bags<input_ports>::type) {}
  void confluence_transition(
      TIME, typename cadmium::make_message_bags<input_ports>::type) {}
  TIME time_advance() const { return TIME{}; }
};

template <typename TIME> struct atomic_no_internal_transition {
  struct in : public cadmium::in_port<int> {};
  struct out : public cadmium::out_port<int> {};
  using state_type = int;
  state_type state = 0;
  using input_ports = std::tuple<in>;
  using output_ports = std::tuple<out>;
  // internal_transition missing
  void
  external_transition(TIME,
                      typename cadmium::make_message_bags<input_ports>::type) {}
  void confluence_transition(
      TIME, typename cadmium::make_message_bags<input_ports>::type) {}
  typename cadmium::make_message_bags<output_ports>::type output() const {
    return {};
  }
  TIME time_advance() const { return TIME{}; }
};

template <typename TIME> struct atomic_no_external_transition {
  struct in : public cadmium::in_port<int> {};
  struct out : public cadmium::out_port<int> {};
  using state_type = int;
  state_type state = 0;
  using input_ports = std::tuple<in>;
  using output_ports = std::tuple<out>;
  void internal_transition() {}
  // external_transition missing
  void confluence_transition(
      TIME, typename cadmium::make_message_bags<input_ports>::type) {}
  typename cadmium::make_message_bags<output_ports>::type output() const {
    return {};
  }
  TIME time_advance() const { return TIME{}; }
};

template <typename TIME> struct atomic_no_confluence_transition {
  struct in : public cadmium::in_port<int> {};
  struct out : public cadmium::out_port<int> {};
  using state_type = int;
  state_type state = 0;
  using input_ports = std::tuple<in>;
  using output_ports = std::tuple<out>;
  void internal_transition() {}
  void
  external_transition(TIME,
                      typename cadmium::make_message_bags<input_ports>::type) {}
  // confluence_transition missing
  typename cadmium::make_message_bags<output_ports>::type output() const {
    return {};
  }
  TIME time_advance() const { return TIME{}; }
};

template <typename TIME> struct atomic_no_output_function {
  struct in : public cadmium::in_port<int> {};
  struct out : public cadmium::out_port<int> {};
  using state_type = int;
  state_type state = 0;
  using input_ports = std::tuple<in>;
  using output_ports = std::tuple<out>;
  void internal_transition() {}
  void
  external_transition(TIME,
                      typename cadmium::make_message_bags<input_ports>::type) {}
  void confluence_transition(
      TIME, typename cadmium::make_message_bags<input_ports>::type) {}
  // output() missing
  TIME time_advance() const { return TIME{}; }
};

template <typename TIME> struct atomic_no_time_advance {
  struct in : public cadmium::in_port<int> {};
  struct out : public cadmium::out_port<int> {};
  using state_type = int;
  state_type state = 0;
  using input_ports = std::tuple<in>;
  using output_ports = std::tuple<out>;
  void internal_transition() {}
  void
  external_transition(TIME,
                      typename cadmium::make_message_bags<input_ports>::type) {}
  void confluence_transition(
      TIME, typename cadmium::make_message_bags<input_ports>::type) {}
  typename cadmium::make_message_bags<output_ports>::type output() const {
    return {};
  }
  // time_advance missing
};

template <typename TIME> struct atomic_repeated_input_port {
  struct in1 : public cadmium::in_port<int> {};
  struct in2 : public cadmium::in_port<int> {};
  struct out : public cadmium::out_port<int> {};
  using state_type = int;
  state_type state = 0;
  using input_ports = std::tuple<in1, in2, in1>; // in1 appears twice
  using output_ports = std::tuple<out>;
  void internal_transition() {}
  void
  external_transition(TIME,
                      typename cadmium::make_message_bags<input_ports>::type) {}
  void confluence_transition(
      TIME, typename cadmium::make_message_bags<input_ports>::type) {}
  typename cadmium::make_message_bags<output_ports>::type output() const {
    return {};
  }
  TIME time_advance() const { return TIME{}; }
};

template <typename TIME> struct atomic_repeated_output_port {
  struct in : public cadmium::in_port<int> {};
  struct out : public cadmium::out_port<int> {};
  using state_type = int;
  state_type state = 0;
  using input_ports = std::tuple<in>;
  using output_ports = std::tuple<out, out>; // out appears twice
  void internal_transition() {}
  void
  external_transition(TIME,
                      typename cadmium::make_message_bags<input_ports>::type) {}
  void confluence_transition(
      TIME, typename cadmium::make_message_bags<input_ports>::type) {}
  typename cadmium::make_message_bags<output_ports>::type output() const {
    return {};
  }
  TIME time_advance() const { return TIME{}; }
};

// ═══════════════════════════════════════════════════════════════════════════
// Valid coupled models
// ═══════════════════════════════════════════════════════════════════════════

template <typename TIME>
using floating_passive = cadmium::basic_models::pdevs::passive<float, TIME>;
using floating_passive_defs = cadmium::basic_models::pdevs::passive_defs<float>;

template <typename TIME>
using floating_accumulator =
    cadmium::basic_models::pdevs::accumulator<float, TIME>;
using floating_accumulator_defs =
    cadmium::basic_models::pdevs::accumulator_defs<float>;

template <typename TIME>
using floating_generator_base =
    cadmium::basic_models::pdevs::generator<float, TIME>;
using floating_generator_defs =
    cadmium::basic_models::pdevs::generator_defs<float>;

template <typename TIME>
struct floating_generator : public floating_generator_base<TIME> {
  float period() const override { return 1.0f; }
  float output_message() const override { return 1.0f; }
};

// shared ports
struct p_in1 : public cadmium::in_port<float> {};
struct p_in2 : public cadmium::in_port<float> {};
struct p_out : public cadmium::out_port<float> {};

// empty coupled
template <typename TIME>
using empty_coupled = cadmium::modeling::pdevs::coupled_model<
    TIME, std::tuple<>, std::tuple<>, cadmium::modeling::models_tuple<>,
    std::tuple<>, std::tuple<>, std::tuple<>>;

// coupled of atomics: generator → accumulator, generator/accumulator → passive
using coa_sub =
    cadmium::modeling::models_tuple<floating_generator, floating_accumulator,
                                    floating_passive>;
using coa_eics = std::tuple<
    cadmium::modeling::EIC<p_in1, floating_passive, floating_passive_defs::in>,
    cadmium::modeling::EIC<p_in2, floating_passive, floating_passive_defs::in>>;
using coa_eocs =
    std::tuple<cadmium::modeling::EOC<floating_accumulator,
                                      floating_accumulator_defs::sum, p_out>,
               cadmium::modeling::EOC<floating_generator,
                                      floating_generator_defs::out, p_out>>;
using coa_ics = std::tuple<
    cadmium::modeling::IC<floating_generator, floating_generator_defs::out,
                          floating_accumulator, floating_accumulator_defs::add>,
    cadmium::modeling::IC<floating_accumulator, floating_accumulator_defs::sum,
                          floating_passive, floating_passive_defs::in>>;
template <typename TIME>
using coupled_of_atomics =
    cadmium::modeling::pdevs::coupled_model<TIME, std::tuple<p_in1, p_in2>,
                                            std::tuple<p_out>, coa_sub,
                                            coa_eics, coa_eocs, coa_ics>;

// nested coupleds: C1/C2/C3 each wrap a passive, C_top wraps C1/C2/C3
using nested_sub = cadmium::modeling::models_tuple<floating_passive>;
using nested_eic = std::tuple<
    cadmium::modeling::EIC<p_in1, floating_passive, floating_passive_defs::in>>;

template <typename TIME>
struct C1 : public cadmium::modeling::pdevs::coupled_model<
                TIME, std::tuple<p_in1, p_in2>, std::tuple<p_out>, nested_sub,
                nested_eic, std::tuple<>, std::tuple<>> {};
template <typename TIME>
struct C2 : public cadmium::modeling::pdevs::coupled_model<
                TIME, std::tuple<p_in1, p_in2>, std::tuple<p_out>, nested_sub,
                nested_eic, std::tuple<>, std::tuple<>> {};
template <typename TIME>
struct C3 : public cadmium::modeling::pdevs::coupled_model<
                TIME, std::tuple<p_in1, p_in2>, std::tuple<p_out>, nested_sub,
                nested_eic, std::tuple<>, std::tuple<>> {};

using top_sub = cadmium::modeling::models_tuple<C1, C2, C3>;
using top_eics = std::tuple<cadmium::modeling::EIC<p_in1, C1, p_in1>,
                            cadmium::modeling::EIC<p_in2, C2, p_in1>>;
template <typename TIME>
using coupled_of_coupleds = cadmium::modeling::pdevs::coupled_model<
    TIME, std::tuple<p_in1, p_in2>, std::tuple<p_out>, top_sub, top_eics,
    std::tuple<>, std::tuple<>>;

// mixed: C1, C2, passive at top level
using mixed_sub = cadmium::modeling::models_tuple<C1, C2, floating_passive>;
using mixed_eics = std::tuple<
    cadmium::modeling::EIC<p_in1, C1, p_in1>,
    cadmium::modeling::EIC<p_in2, C2, p_in1>,
    cadmium::modeling::EIC<p_in2, floating_passive, floating_passive_defs::in>>;
template <typename TIME>
using coupled_of_mixed = cadmium::modeling::pdevs::coupled_model<
    TIME, std::tuple<p_in1, p_in2>, std::tuple<p_out>, mixed_sub, mixed_eics,
    std::tuple<>, std::tuple<>>;

// ═══════════════════════════════════════════════════════════════════════════
// Invalid coupled models — one violation each
// ═══════════════════════════════════════════════════════════════════════════

// EIC type mismatch: external port is float, submodel port expects int
template <typename TIME>
using passive_int = cadmium::basic_models::pdevs::passive<int, TIME>;
using passive_int_in = cadmium::basic_models::pdevs::passive_defs<int>::in;

struct ext_float_in : public cadmium::in_port<float> {};
using bad_eic_sub = cadmium::modeling::models_tuple<passive_int>;
using bad_eic_eics = std::tuple<
    cadmium::modeling::EIC<ext_float_in, passive_int, passive_int_in>>;
template <typename TIME>
using coupled_eic_type_mismatch = cadmium::modeling::pdevs::coupled_model<
    TIME, std::tuple<ext_float_in>, std::tuple<>, bad_eic_sub, bad_eic_eics,
    std::tuple<>, std::tuple<>>;

// EOC type mismatch: generator output is float, external output port expects
// int
struct ext_int_out : public cadmium::out_port<int> {};
using bad_eoc_sub = cadmium::modeling::models_tuple<floating_generator>;
using bad_eoc_eocs = std::tuple<cadmium::modeling::EOC<
    floating_generator, floating_generator_defs::out, ext_int_out>>;
template <typename TIME>
using coupled_eoc_type_mismatch = cadmium::modeling::pdevs::coupled_model<
    TIME, std::tuple<>, std::tuple<ext_int_out>, bad_eoc_sub, std::tuple<>,
    bad_eoc_eocs, std::tuple<>>;

// IC type mismatch: generator output is float, passive_int::in expects int
using bad_ic_sub =
    cadmium::modeling::models_tuple<floating_generator, passive_int>;
using bad_ic_ics = std::tuple<
    cadmium::modeling::IC<floating_generator, floating_generator_defs::out,
                          passive_int, passive_int_in>>;
template <typename TIME>
using coupled_ic_type_mismatch =
    cadmium::modeling::pdevs::coupled_model<TIME, std::tuple<>, std::tuple<>,
                                            bad_ic_sub, std::tuple<>,
                                            std::tuple<>, bad_ic_ics>;

// repeated coupled input port
struct dup_in : public cadmium::in_port<float> {};
template <typename TIME>
using coupled_repeated_input = cadmium::modeling::pdevs::coupled_model<
    TIME, std::tuple<dup_in, dup_in>, std::tuple<>,
    cadmium::modeling::models_tuple<>, std::tuple<>, std::tuple<>,
    std::tuple<>>;

// repeated coupled output port
struct dup_out : public cadmium::out_port<float> {};
template <typename TIME>
using coupled_repeated_output = cadmium::modeling::pdevs::coupled_model<
    TIME, std::tuple<>, std::tuple<dup_out, dup_out>,
    cadmium::modeling::models_tuple<>, std::tuple<>, std::tuple<>,
    std::tuple<>>;

} // anonymous namespace

// ═══════════════════════════════════════════════════════════════════════════
// AtomicModel concept — positive
// ═══════════════════════════════════════════════════════════════════════════

SCENARIO("minimal valid atomic model satisfies AtomicModel",
         "[pdevs][concepts][atomic][positive]") {
  STATIC_REQUIRE(AtomicModel<valid_atomic<float>, float>);
}

SCENARIO("valid atomic with multiple distinct ports satisfies AtomicModel",
         "[pdevs][concepts][atomic][positive]") {
  STATIC_REQUIRE(AtomicModel<valid_atomic_multi_port<float>, float>);
}

SCENARIO("pdevs generator satisfies AtomicModel",
         "[pdevs][concepts][atomic][positive]") {
  STATIC_REQUIRE(AtomicModel<floating_generator<float>, float>);
}

SCENARIO("pdevs passive satisfies AtomicModel",
         "[pdevs][concepts][atomic][positive]") {
  STATIC_REQUIRE(AtomicModel<floating_passive<float>, float>);
}

SCENARIO("pdevs accumulator satisfies AtomicModel",
         "[pdevs][concepts][atomic][positive]") {
  STATIC_REQUIRE(AtomicModel<floating_accumulator<float>, float>);
}

// ═══════════════════════════════════════════════════════════════════════════
// AtomicModel concept — negative
// ═══════════════════════════════════════════════════════════════════════════

SCENARIO("model missing state_type does not satisfy AtomicModel",
         "[pdevs][concepts][atomic][negative]") {
  STATIC_REQUIRE(!AtomicModel<atomic_no_state_type<float>, float>);
}

SCENARIO("model missing state variable does not satisfy AtomicModel",
         "[pdevs][concepts][atomic][negative]") {
  STATIC_REQUIRE(!AtomicModel<atomic_no_state_var<float>, float>);
}

SCENARIO("model missing input_ports does not satisfy AtomicModel",
         "[pdevs][concepts][atomic][negative]") {
  STATIC_REQUIRE(!AtomicModel<atomic_no_input_ports<float>, float>);
}

SCENARIO("model missing output_ports does not satisfy AtomicModel",
         "[pdevs][concepts][atomic][negative]") {
  STATIC_REQUIRE(!AtomicModel<atomic_no_output_ports<float>, float>);
}

SCENARIO("model missing internal_transition does not satisfy AtomicModel",
         "[pdevs][concepts][atomic][negative]") {
  STATIC_REQUIRE(!AtomicModel<atomic_no_internal_transition<float>, float>);
}

SCENARIO("model missing external_transition does not satisfy AtomicModel",
         "[pdevs][concepts][atomic][negative]") {
  STATIC_REQUIRE(!AtomicModel<atomic_no_external_transition<float>, float>);
}

SCENARIO("model missing confluence_transition does not satisfy AtomicModel",
         "[pdevs][concepts][atomic][negative]") {
  STATIC_REQUIRE(!AtomicModel<atomic_no_confluence_transition<float>, float>);
}

SCENARIO("model missing output function does not satisfy AtomicModel",
         "[pdevs][concepts][atomic][negative]") {
  STATIC_REQUIRE(!AtomicModel<atomic_no_output_function<float>, float>);
}

SCENARIO("model missing time_advance does not satisfy AtomicModel",
         "[pdevs][concepts][atomic][negative]") {
  STATIC_REQUIRE(!AtomicModel<atomic_no_time_advance<float>, float>);
}

SCENARIO("model with repeated input port type does not satisfy AtomicModel",
         "[pdevs][concepts][atomic][negative]") {
  STATIC_REQUIRE(!AtomicModel<atomic_repeated_input_port<float>, float>);
}

SCENARIO("model with repeated output port type does not satisfy AtomicModel",
         "[pdevs][concepts][atomic][negative]") {
  STATIC_REQUIRE(!AtomicModel<atomic_repeated_output_port<float>, float>);
}

// ═══════════════════════════════════════════════════════════════════════════
// CoupledModel concept — positive
// ═══════════════════════════════════════════════════════════════════════════

SCENARIO("empty coupled model satisfies CoupledModel",
         "[pdevs][concepts][coupled][positive]") {
  STATIC_REQUIRE(CoupledModel<empty_coupled<float>>);
}

SCENARIO("coupled of all atomics satisfies CoupledModel",
         "[pdevs][concepts][coupled][positive]") {
  STATIC_REQUIRE(CoupledModel<coupled_of_atomics<float>>);
}

SCENARIO("coupled of nested coupleds satisfies CoupledModel",
         "[pdevs][concepts][coupled][positive]") {
  STATIC_REQUIRE(CoupledModel<coupled_of_coupleds<float>>);
}

SCENARIO("coupled of mixed atomics and coupleds satisfies CoupledModel",
         "[pdevs][concepts][coupled][positive]") {
  STATIC_REQUIRE(CoupledModel<coupled_of_mixed<float>>);
}

SCENARIO("an atomic model does not satisfy CoupledModel",
         "[pdevs][concepts][coupled][negative]") {
  STATIC_REQUIRE(!CoupledModel<valid_atomic<float>>);
}

// ═══════════════════════════════════════════════════════════════════════════
// CoupledModel concept — negative: coupling violations
// ═══════════════════════════════════════════════════════════════════════════

SCENARIO("EIC with mismatched port message types does not satisfy CoupledModel",
         "[pdevs][concepts][coupled][negative]") {
  STATIC_REQUIRE(!CoupledModel<coupled_eic_type_mismatch<float>>);
}

SCENARIO("EOC with mismatched port message types does not satisfy CoupledModel",
         "[pdevs][concepts][coupled][negative]") {
  STATIC_REQUIRE(!CoupledModel<coupled_eoc_type_mismatch<float>>);
}

SCENARIO("IC with mismatched port message types does not satisfy CoupledModel",
         "[pdevs][concepts][coupled][negative]") {
  STATIC_REQUIRE(!CoupledModel<coupled_ic_type_mismatch<float>>);
}

SCENARIO(
    "coupled model with repeated input port type does not satisfy CoupledModel",
    "[pdevs][concepts][coupled][negative]") {
  STATIC_REQUIRE(!CoupledModel<coupled_repeated_input<float>>);
}

SCENARIO("coupled model with repeated output port type does not satisfy "
         "CoupledModel",
         "[pdevs][concepts][coupled][negative]") {
  STATIC_REQUIRE(!CoupledModel<coupled_repeated_output<float>>);
}

// ═══════════════════════════════════════════════════════════════════════════
// DEVS AtomicModel concept — fixture types
// ═══════════════════════════════════════════════════════════════════════════

namespace devs_fixtures {

template <typename TIME> struct valid_devs {
  struct in : public cadmium::in_port<int> {};
  struct out : public cadmium::out_port<int> {};
  using state_type = int;
  state_type state = 0;
  using input_ports = std::tuple<in>;
  using output_ports = std::tuple<out>;
  void internal_transition() {}
  void
  external_transition(TIME,
                      typename cadmium::make_message_box<input_ports>::type) {}
  typename cadmium::make_message_box<output_ports>::type output() const {
    return {};
  }
  TIME time_advance() const { return TIME{}; }
};

template <typename TIME> struct devs_no_state_type {
  struct in : public cadmium::in_port<int> {};
  struct out : public cadmium::out_port<int> {};
  int state = 0;
  using input_ports = std::tuple<in>;
  using output_ports = std::tuple<out>;
  void internal_transition() {}
  void
  external_transition(TIME,
                      typename cadmium::make_message_box<input_ports>::type) {}
  typename cadmium::make_message_box<output_ports>::type output() const {
    return {};
  }
  TIME time_advance() const { return TIME{}; }
};

template <typename TIME> struct devs_no_state_var {
  struct in : public cadmium::in_port<int> {};
  struct out : public cadmium::out_port<int> {};
  using state_type = int;
  using input_ports = std::tuple<in>;
  using output_ports = std::tuple<out>;
  void internal_transition() {}
  void
  external_transition(TIME,
                      typename cadmium::make_message_box<input_ports>::type) {}
  typename cadmium::make_message_box<output_ports>::type output() const {
    return {};
  }
  TIME time_advance() const { return TIME{}; }
};

template <typename TIME> struct devs_no_input_ports {
  struct out : public cadmium::out_port<int> {};
  using state_type = int;
  state_type state = 0;
  using output_ports = std::tuple<out>;
  void internal_transition() {}
  typename cadmium::make_message_box<output_ports>::type output() const {
    return {};
  }
  TIME time_advance() const { return TIME{}; }
};

template <typename TIME> struct devs_no_output_ports {
  struct in : public cadmium::in_port<int> {};
  using state_type = int;
  state_type state = 0;
  using input_ports = std::tuple<in>;
  void internal_transition() {}
  void
  external_transition(TIME,
                      typename cadmium::make_message_box<input_ports>::type) {}
  TIME time_advance() const { return TIME{}; }
};

template <typename TIME> struct devs_no_internal_transition {
  struct in : public cadmium::in_port<int> {};
  struct out : public cadmium::out_port<int> {};
  using state_type = int;
  state_type state = 0;
  using input_ports = std::tuple<in>;
  using output_ports = std::tuple<out>;
  void
  external_transition(TIME,
                      typename cadmium::make_message_box<input_ports>::type) {}
  typename cadmium::make_message_box<output_ports>::type output() const {
    return {};
  }
  TIME time_advance() const { return TIME{}; }
};

template <typename TIME> struct devs_no_external_transition {
  struct in : public cadmium::in_port<int> {};
  struct out : public cadmium::out_port<int> {};
  using state_type = int;
  state_type state = 0;
  using input_ports = std::tuple<in>;
  using output_ports = std::tuple<out>;
  void internal_transition() {}
  typename cadmium::make_message_box<output_ports>::type output() const {
    return {};
  }
  TIME time_advance() const { return TIME{}; }
};

template <typename TIME> struct devs_no_output_function {
  struct in : public cadmium::in_port<int> {};
  struct out : public cadmium::out_port<int> {};
  using state_type = int;
  state_type state = 0;
  using input_ports = std::tuple<in>;
  using output_ports = std::tuple<out>;
  void internal_transition() {}
  void
  external_transition(TIME,
                      typename cadmium::make_message_box<input_ports>::type) {}
  TIME time_advance() const { return TIME{}; }
};

template <typename TIME> struct devs_no_time_advance {
  struct in : public cadmium::in_port<int> {};
  struct out : public cadmium::out_port<int> {};
  using state_type = int;
  state_type state = 0;
  using input_ports = std::tuple<in>;
  using output_ports = std::tuple<out>;
  void internal_transition() {}
  void
  external_transition(TIME,
                      typename cadmium::make_message_box<input_ports>::type) {}
  typename cadmium::make_message_box<output_ports>::type output() const {
    return {};
  }
};

template <typename TIME> struct devs_repeated_input_port {
  struct in1 : public cadmium::in_port<int> {};
  struct out : public cadmium::out_port<int> {};
  using state_type = int;
  state_type state = 0;
  using input_ports = std::tuple<in1, in1>;
  using output_ports = std::tuple<out>;
  void internal_transition() {}
  void
  external_transition(TIME,
                      typename cadmium::make_message_box<input_ports>::type) {}
  typename cadmium::make_message_box<output_ports>::type output() const {
    return {};
  }
  TIME time_advance() const { return TIME{}; }
};

template <typename TIME> struct devs_repeated_output_port {
  struct in : public cadmium::in_port<int> {};
  struct out : public cadmium::out_port<int> {};
  using state_type = int;
  state_type state = 0;
  using input_ports = std::tuple<in>;
  using output_ports = std::tuple<out, out>;
  void internal_transition() {}
  void
  external_transition(TIME,
                      typename cadmium::make_message_box<input_ports>::type) {}
  typename cadmium::make_message_box<output_ports>::type output() const {
    return {};
  }
  TIME time_advance() const { return TIME{}; }
};

} // namespace devs_fixtures

// ═══════════════════════════════════════════════════════════════════════════
// DEVS AtomicModel concept — positive
// ═══════════════════════════════════════════════════════════════════════════

SCENARIO("minimal valid DEVS atomic model satisfies devs::AtomicModel",
         "[devs][concepts][atomic][positive]") {
  STATIC_REQUIRE(
      devs_concepts::AtomicModel<devs_fixtures::valid_devs<float>, float>);
}

SCENARIO("DEVS generator satisfies devs::AtomicModel",
         "[devs][concepts][atomic][positive]") {
  STATIC_REQUIRE(devs_concepts::AtomicModel<
                 cadmium::basic_models::devs::generator<float, float>, float>);
}

SCENARIO("DEVS passive satisfies devs::AtomicModel",
         "[devs][concepts][atomic][positive]") {
  STATIC_REQUIRE(devs_concepts::AtomicModel<
                 cadmium::basic_models::devs::passive<float, float>, float>);
}

// ═══════════════════════════════════════════════════════════════════════════
// DEVS AtomicModel concept — negative
// ═══════════════════════════════════════════════════════════════════════════

SCENARIO("DEVS model missing state_type does not satisfy devs::AtomicModel",
         "[devs][concepts][atomic][negative]") {
  STATIC_REQUIRE(
      !devs_concepts::AtomicModel<devs_fixtures::devs_no_state_type<float>,
                                  float>);
}

SCENARIO("DEVS model missing state variable does not satisfy devs::AtomicModel",
         "[devs][concepts][atomic][negative]") {
  STATIC_REQUIRE(
      !devs_concepts::AtomicModel<devs_fixtures::devs_no_state_var<float>,
                                  float>);
}

SCENARIO("DEVS model missing input_ports does not satisfy devs::AtomicModel",
         "[devs][concepts][atomic][negative]") {
  STATIC_REQUIRE(
      !devs_concepts::AtomicModel<devs_fixtures::devs_no_input_ports<float>,
                                  float>);
}

SCENARIO("DEVS model missing output_ports does not satisfy devs::AtomicModel",
         "[devs][concepts][atomic][negative]") {
  STATIC_REQUIRE(
      !devs_concepts::AtomicModel<devs_fixtures::devs_no_output_ports<float>,
                                  float>);
}

SCENARIO(
    "DEVS model missing internal_transition does not satisfy devs::AtomicModel",
    "[devs][concepts][atomic][negative]") {
  STATIC_REQUIRE(!devs_concepts::AtomicModel<
                 devs_fixtures::devs_no_internal_transition<float>, float>);
}

SCENARIO(
    "DEVS model missing external_transition does not satisfy devs::AtomicModel",
    "[devs][concepts][atomic][negative]") {
  STATIC_REQUIRE(!devs_concepts::AtomicModel<
                 devs_fixtures::devs_no_external_transition<float>, float>);
}

SCENARIO(
    "DEVS model missing output function does not satisfy devs::AtomicModel",
    "[devs][concepts][atomic][negative]") {
  STATIC_REQUIRE(
      !devs_concepts::AtomicModel<devs_fixtures::devs_no_output_function<float>,
                                  float>);
}

SCENARIO("DEVS model missing time_advance does not satisfy devs::AtomicModel",
         "[devs][concepts][atomic][negative]") {
  STATIC_REQUIRE(
      !devs_concepts::AtomicModel<devs_fixtures::devs_no_time_advance<float>,
                                  float>);
}

SCENARIO("DEVS model with repeated input port type does not satisfy "
         "devs::AtomicModel",
         "[devs][concepts][atomic][negative]") {
  STATIC_REQUIRE(!devs_concepts::AtomicModel<
                 devs_fixtures::devs_repeated_input_port<float>, float>);
}

SCENARIO("DEVS model with repeated output port type does not satisfy "
         "devs::AtomicModel",
         "[devs][concepts][atomic][negative]") {
  STATIC_REQUIRE(!devs_concepts::AtomicModel<
                 devs_fixtures::devs_repeated_output_port<float>, float>);
}
