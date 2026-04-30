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

#ifndef CADMIUM_ATOMIC_MODEL_ASSERT_HPP
#define CADMIUM_ATOMIC_MODEL_ASSERT_HPP

#include <cadmium/concepts/concept_helpers.hpp>
#include <cadmium/concepts/pdevs_concepts.hpp>
#include <cadmium/modeling/message_bag.hpp>
#include <cadmium/modeling/message_box.hpp>

namespace cadmium::model_checks {
namespace pdevs {
template <typename FLOATING_MODEL>
constexpr void atomic_model_float_time_assert() {
  static_assert(
      cadmium::concepts::pdevs::AtomicModel<FLOATING_MODEL, float>,
      "MODEL does not satisfy the AtomicModel concept: check state_type, "
      "input_ports, output_ports, port uniqueness, and all five PDEVS "
      "functions");
}

template <template <typename> class MODEL>
constexpr void atomic_model_assert() {
  atomic_model_float_time_assert<MODEL<float>>();
}
} // namespace pdevs

namespace devs {
template <typename FLOATING_MODEL>
constexpr void atomic_model_float_time_assert() {
  using ip = typename FLOATING_MODEL::input_ports;
  using op = typename FLOATING_MODEL::output_ports;
  using ip_box = typename make_message_box<ip>::type;
  using op_box = typename make_message_box<op>::type;
  static_assert(check_unique_elem_types<ip>::value(),
                "ambiguous port name in input ports");
  static_assert(check_unique_elem_types<op>::value(),
                "ambiguous port name in output ports");
  static_assert(std::is_same<typename FLOATING_MODEL::state_type,
                             decltype(FLOATING_MODEL{}.state)>::value,
                "state is undefined or has the wrong type");
  static_assert(
      std::is_same<decltype(std::declval<FLOATING_MODEL>().output()), op_box>(),
      "Output function does not exist or does not return the right message "
      "bags");
  static_assert(
      std::is_same<decltype(std::declval<FLOATING_MODEL>().external_transition(
                       0.0, ip_box{})),
                   void>(),
      "External transition function undefined");
  static_assert(std::is_same<decltype(std::declval<FLOATING_MODEL>()
                                          .internal_transition()),
                             void>(),
                "Internal transition function undefined");
  static_assert(
      std::is_same<decltype(std::declval<FLOATING_MODEL>().time_advance()),
                   float>(),
      "Time advance function does not exist or does not return the right type "
      "of time");
}

template <template <typename> class MODEL>
constexpr void atomic_model_assert() {
  atomic_model_float_time_assert<MODEL<float>>();
}
} // namespace devs
} // namespace cadmium::model_checks

#endif // CADMIUM_ATOMIC_MODEL_ASSERT_HPP
