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

#ifndef CADMIUM_PDEVS_COORDINATOR_H
#define CADMIUM_PDEVS_COORDINATOR_H

#include <limits>

#include <cadmium/concepts/pdevs_concepts.hpp>
#include <cadmium/engine/pdevs_engine_helpers.hpp>
#include <cadmium/engine/pdevs_simulator.hpp>
#include <cadmium/logger/cadmium_log.hpp>
#include <cadmium/logger/common_loggers_helpers.hpp>
#include <cadmium/modeling/coupling.hpp>
#include <cadmium/modeling/message_bag.hpp>

namespace cadmium {
namespace engine {

template <template <typename T> class MODEL, typename TIME>
  requires cadmium::concepts::pdevs::CoupledModel<MODEL<TIME>>
class coordinator {

  template <typename P>
  using submodels_type = typename MODEL<TIME>::template models<P>;
  using in_bags_type =
      typename make_message_bags<typename MODEL<TIME>::input_ports>::type;
  using out_bags_type =
      typename make_message_bags<typename MODEL<TIME>::output_ports>::type;
  using subcoordinators_type =
      typename coordinate_tuple<TIME, submodels_type>::type;
  using eic = typename MODEL<TIME>::external_input_couplings;
  using eoc = typename MODEL<TIME>::external_output_couplings;
  using ic = typename MODEL<TIME>::internal_couplings;

  TIME _last{};
  TIME _next{};
  subcoordinators_type _subcoordinators;
  std::string _model_id{};

public:
  // TODO: set boxes back to private
  in_bags_type _inbox{};
  out_bags_type _outbox{};

  using model_type = MODEL<TIME>;

  void init(TIME t) {
    _model_id = cadmium::logger::model_type_name<MODEL<TIME>>();
    cadmium::log::emit(cadmium::log::level::info, "coor_info_init", _model_id,
                       cadmium::log::to_sim_double(t));

    _last = t;
    cadmium::engine::init_subcoordinators<TIME, subcoordinators_type>(
        t, _subcoordinators);
    _next = cadmium::engine::min_next_in_tuple<subcoordinators_type>(
        _subcoordinators);
  }

  TIME next() const noexcept { return _next; }

  void collect_outputs(const TIME &t) {
    cadmium::log::emit(cadmium::log::level::debug, "coor_info_collect",
                       _model_id, cadmium::log::to_sim_double(t));

    if (_next < t) {
      throw std::domain_error(
          "Trying to obtain output when not internal event is scheduled");
    } else if (_next == t) {
      cadmium::log::emit(cadmium::log::level::debug, "coor_routing_eoc_collect",
                         _model_id, cadmium::log::to_sim_double(t));
      cadmium::engine::collect_outputs_in_subcoordinators<TIME,
                                                          subcoordinators_type>(
          t, _subcoordinators);
      _outbox = collect_messages_by_eoc<TIME, eoc, out_bags_type,
                                        subcoordinators_type>(_subcoordinators);
    }
  }

  out_bags_type outbox() const noexcept { return _outbox; }

  void advance_simulation(const TIME &t) {
    _outbox = out_bags_type{};

    cadmium::log::emit(cadmium::log::level::debug, "coor_info_advance",
                       _model_id, cadmium::log::to_sim_double(t));

    if (_next < t || t < _last) {
      throw std::domain_error(
          "Trying to obtain output when out of the advance time scope");
    } else {
      cadmium::log::emit(cadmium::log::level::debug, "coor_routing_ic_collect",
                         _model_id, cadmium::log::to_sim_double(t));
      cadmium::engine::route_internal_coupled_messages_on_subcoordinators<
          TIME, subcoordinators_type, ic>(t, _subcoordinators);

      cadmium::log::emit(cadmium::log::level::debug, "coor_routing_eic_collect",
                         _model_id, cadmium::log::to_sim_double(t));
      cadmium::engine::route_external_input_coupled_messages_on_subcoordinators<
          TIME, in_bags_type, subcoordinators_type, eic>(t, _inbox,
                                                         _subcoordinators);

      cadmium::engine::advance_simulation_in_subengines<TIME,
                                                        subcoordinators_type>(
          t, _subcoordinators);

      _last = t;
      _next = cadmium::engine::min_next_in_tuple<subcoordinators_type>(
          _subcoordinators);
      _inbox = in_bags_type{};
    }
  }
};

} // namespace engine
} // namespace cadmium

#endif // CADMIUM_PDEVS_COORDINATOR_H
