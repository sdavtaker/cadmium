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

#ifndef CADMIUM_PDEVS_SIMULATOR_HPP
#define CADMIUM_PDEVS_SIMULATOR_HPP

#include <sstream>

#include <cadmium/concepts/pdevs_concepts.hpp>
#include <cadmium/engine/pdevs_engine_helpers.hpp>
#include <cadmium/logger/cadmium_log.hpp>
#include <cadmium/logger/common_loggers_helpers.hpp>
#include <cadmium/modeling/message_bag.hpp>

namespace cadmium {
namespace engine {

template <template <typename T> class MODEL, typename TIME>
  requires cadmium::concepts::pdevs::AtomicModel<MODEL<TIME>, TIME>
class simulator {

  using input_ports = typename MODEL<TIME>::input_ports;
  using output_ports = typename MODEL<TIME>::output_ports;
  using in_bags_type = typename make_message_bags<input_ports>::type;
  using out_bags_type = typename make_message_bags<output_ports>::type;

  MODEL<TIME> _model;
  TIME _last{};
  TIME _next{};
  std::string _model_id{};
  in_bags_type _inbox{};
  out_bags_type _outbox{};

public:
  using model_type = MODEL<TIME>;

  void init(TIME initial_time) {
    _model_id = cadmium::logger::model_type_name<model_type>();
    cadmium::log::emit(cadmium::log::level::info, "sim_info_init", _model_id,
                       cadmium::log::to_sim_double(initial_time));

    _last = initial_time;
    _next = initial_time + _model.time_advance();

    std::ostringstream oss;
    oss << _model.state;
    cadmium::log::emit(cadmium::log::level::debug, "sim_state",
                       _model_id + " " + oss.str(),
                       cadmium::log::to_sim_double(initial_time));
  }

  TIME next() const noexcept { return _next; }

  void collect_outputs(const TIME &t) {
    cadmium::log::emit(cadmium::log::level::debug, "sim_info_collect",
                       _model_id, cadmium::log::to_sim_double(t));

    _inbox = in_bags_type{};

    if (_next < t) {
      throw std::domain_error(
          "Trying to obtain output when not internal event is scheduled");
    } else if (_next == t) {
      _outbox = _model.output();
    } else {
      _outbox = out_bags_type();
    }

    std::ostringstream oss;
    cadmium::logger::print_messages_by_port(oss, _outbox);
    cadmium::log::emit(cadmium::log::level::debug, "sim_messages_collect",
                       _model_id + " " + oss.str(),
                       cadmium::log::to_sim_double(t));
  }

  const out_bags_type &outbox() const noexcept { return _outbox; }

  void inbox(in_bags_type in) noexcept { _inbox = std::move(in); }

  template <typename PORT>
  const std::vector<typename PORT::message_type> &outbox_port() const noexcept {
    return cadmium::get_messages<PORT>(_outbox);
  }

  template <typename PORT>
  void append_to_inbox(const std::vector<typename PORT::message_type> &msgs) {
    auto &bag = cadmium::get_messages<PORT>(_inbox);
    bag.insert(bag.end(), msgs.begin(), msgs.end());
  }

  void advance_simulation(TIME t) {
    _outbox = out_bags_type{};

    cadmium::log::emit(cadmium::log::level::debug, "sim_info_advance",
                       _model_id, cadmium::log::to_sim_double(t));

    if (t < _last) {
      throw std::domain_error("Event received for executing in the past of "
                              "current simulation time");
    } else if (_next < t) {
      throw std::domain_error(
          "Event received for executing after next internal event");
    } else {
      if (!cadmium::engine::all_bags_empty(_inbox)) {
        if (t == _next) {
          _model.confluence_transition(t - _last, _inbox);
        } else {
          _model.external_transition(t - _last, _inbox);
        }
        _last = t;
        _next = _last + _model.time_advance();
        _inbox = in_bags_type{};
      } else {
        if (t == _next) {
          _model.internal_transition();
          _last = t;
          _next = _last + _model.time_advance();
        }
        // t != _next with empty inbox: no-op (all models iterated, not FEL)
      }
    }

    std::ostringstream oss;
    oss << _model.state;
    cadmium::log::emit(cadmium::log::level::debug, "sim_state",
                       _model_id + " " + oss.str(),
                       cadmium::log::to_sim_double(t));
  }
};

} // namespace engine
} // namespace cadmium

#endif // CADMIUM_PDEVS_SIMULATOR_HPP
