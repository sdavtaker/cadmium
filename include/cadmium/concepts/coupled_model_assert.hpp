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

#ifndef CADMIUM_COUPLED_MODEL_ASSERT_HPP
#define CADMIUM_COUPLED_MODEL_ASSERT_HPP

#include <cadmium/concepts/atomic_model_assert.hpp>
#include <cadmium/concepts/concept_helpers.hpp>
#include <cadmium/modeling/message_bag.hpp>
#include <cadmium/modeling/ports.hpp>
#include <type_traits>

namespace cadmium::model_checks {
// static assert over the EIC descriptions
template <typename IN, typename EICs, int S> struct assert_each_eic {
  static constexpr bool value() {
    using EP = std::tuple_element_t<S - 1, EICs>::external_input_port;
    using SUBMODEL =
        std::tuple_element_t<S - 1, EICs>::template submodel<float>;
    using IP = std::tuple_element_t<S - 1, EICs>::submodel_input_port;
    static_assert(EP::kind == port_kind::in,
                  "The external port in a EIC is not an input port");
    static_assert(IP::kind == port_kind::in,
                  "The internal port in a EIC is not an input port");
    static_assert(
        has_port_in_tuple<IP, typename SUBMODEL::input_ports>::value(),
        "Internal port in EIC is not defined in the submodel's input_ports");
    static_assert(has_port_in_tuple<EP, IN>::value(),
                  "External port in EIC is not defined in the coupled model's "
                  "input_ports");
    static_assert(
        std::is_same_v<typename EP::message_type, typename IP::message_type>,
        "The message type does not match in EIC description");
    return assert_each_eic<IN, EICs, S - 1>::value();
  }
};

template <typename IN, typename EICs> struct assert_each_eic<IN, EICs, 0> {
  static constexpr bool value() { return true; }
};

template <typename IN, typename EICs> constexpr void assert_eic(IN, EICs) {
  assert_each_eic<IN, EICs, std::tuple_size_v<EICs>>::value();
  static_assert(is_tuple<EICs>(), "EIC is not a tuple");
}

template <typename OUT, typename EOCs, int S> struct assert_each_eoc {
  static constexpr bool value() {
    using EP = std::tuple_element_t<S - 1, EOCs>::external_output_port;
    using SUBMODEL =
        std::tuple_element_t<S - 1, EOCs>::template submodel<float>;
    using IP = std::tuple_element_t<S - 1, EOCs>::submodel_output_port;
    static_assert(EP::kind == port_kind::out,
                  "The external port in a EOC is not an output port");
    static_assert(IP::kind == port_kind::out,
                  "The internal port in a EOC is not an output port");
    static_assert(
        has_port_in_tuple<IP, typename SUBMODEL::output_ports>::value(),
        "Internal port in EOC is not defined in the submodel's output_ports");
    static_assert(has_port_in_tuple<EP, OUT>::value(),
                  "External port in EOC is not defined in the coupled model's "
                  "output_ports");
    static_assert(
        std::is_same_v<typename EP::message_type, typename IP::message_type>,
        "The message type does not match in EOC description");
    return assert_each_eoc<OUT, EOCs, S - 1>::value();
  }
};

template <typename OUT, typename EOCs> struct assert_each_eoc<OUT, EOCs, 0> {
  static constexpr bool value() { return true; }
};

template <typename OUT, typename EOCs> constexpr void assert_eoc(OUT, EOCs) {
  assert_each_eoc<OUT, EOCs, std::tuple_size_v<EOCs>>::value();
  static_assert(is_tuple<EOCs>(), "EOC is not a tuple");
}

template <typename ICs, int S> struct assert_each_ic {
  static constexpr bool value() {
    using FROM_MODEL =
        std::tuple_element_t<S - 1, ICs>::template from_model<float>;
    using FROM_PORT = std::tuple_element_t<S - 1, ICs>::from_model_output_port;
    using TO_MODEL = std::tuple_element_t<S - 1, ICs>::template to_model<float>;
    using TO_PORT = std::tuple_element_t<S - 1, ICs>::to_model_input_port;
    static_assert(FROM_PORT::kind == port_kind::out,
                  "The from_model port in an IC is not an output port");
    static_assert(TO_PORT::kind == port_kind::in,
                  "The to_model port in an IC is not an input port");
    static_assert(has_port_in_tuple<FROM_PORT,
                                    typename FROM_MODEL::output_ports>::value(),
                  "Output port used in IC is not defined in the from_model");
    static_assert(
        has_port_in_tuple<TO_PORT, typename TO_MODEL::input_ports>::value(),
        "Input port used in IC is not defined in the to_model");
    static_assert(std::is_same_v<typename TO_PORT::message_type,
                                 typename FROM_PORT::message_type>,
                  "The message type does not match in IC description");
    static_assert(!std::is_same_v<FROM_MODEL, TO_MODEL>,
                  "The IC detected a coupling-to-self loop");
    return assert_each_ic<ICs, S - 1>::value();
  }
};

template <typename ICs> struct assert_each_ic<ICs, 0> {
  static constexpr bool value() { return true; }
};

template <typename ICs> constexpr void assert_ic(ICs) {
  assert_each_ic<ICs, std::tuple_size_v<ICs>>::value();
  static_assert(is_tuple<ICs>(), "ICs is not a tuple");
}

namespace pdevs {
template <typename MODEL> constexpr void coupled_model_float_time_assert();

template <typename MODELs, int S> struct assert_each_submodel {
  using MODEL = std::tuple_element_t<S - 1, MODELs>;
  // testing if model has to be checked as atomic or coupled
  using atomic_detected = char;
  using coupled_detected = long;

  template <typename M> static atomic_detected test(decltype(&M::time_advance));

  template <typename M> static coupled_detected test(...);

  static constexpr void check() {
    if constexpr (sizeof(test<MODEL>(0)) == sizeof(atomic_detected)) {
      cadmium::model_checks::pdevs::atomic_model_float_time_assert<MODEL>();
    } else {
      coupled_model_float_time_assert<MODEL>();
    }
    assert_each_submodel<MODELs, S - 1>();
  }
};

template <typename MODELs> struct assert_each_submodel<MODELs, 0> {
  static constexpr void check() {}
};

template <typename MODELs> constexpr void assert_submodels() {
  static_assert(is_tuple<MODELs>(), "Submodels is not a tuple");
  assert_each_submodel<MODELs, std::tuple_size_v<MODELs>>::check();
}

template <typename FLOATING_MODEL>
constexpr void coupled_model_float_time_assert() {
  using IP = typename FLOATING_MODEL::input_ports;
  using OP = typename FLOATING_MODEL::output_ports;
  using EICs = typename FLOATING_MODEL::external_input_couplings;
  using EOCs = typename FLOATING_MODEL::external_output_couplings;
  using ICs = typename FLOATING_MODEL::internal_couplings;
  assert_eic(IP{}, EICs{});
  assert_eoc(OP{}, EOCs{});
  assert_ic(ICs{});
  static_assert(check_unique_elem_types<IP>::value(),
                "ambiguous port name in input ports");
  static_assert(check_unique_elem_types<OP>::value(),
                "ambiguous port name in output ports");
  using floating_submodels = typename FLOATING_MODEL::template models<float>;
  static_assert(std::is_constructible_v<floating_submodels>,
                "coupled model cannot be constructed");
  assert_submodels<floating_submodels>();
}

template <template <typename TIME> class MODEL>
// check a template argument is required (for time)
constexpr void coupled_model_assert() {
  using floating_model = MODEL<float>;
  coupled_model_float_time_assert<floating_model>();
}

} // namespace pdevs
} // namespace cadmium::model_checks
#endif // CADMIUM_COUPLED_MODEL_ASSERT_HPP
