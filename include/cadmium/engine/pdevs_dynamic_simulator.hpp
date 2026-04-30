/**
 * Copyright (c) 2017-present, Laouen M. L. Belloli, Damian Vicino
 * Carleton University, Universidad de Buenos Aires, Universite de Nice-Sophia
 * Antipolis All rights reserved.
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

#pragma once

#include <cadmium/engine/pdevs_dynamic_engine.hpp>
#include <cadmium/logger/cadmium_log.hpp>
#include <cadmium/modeling/dynamic_message_bag.hpp>
#include <cadmium/modeling/dynamic_model.hpp>

namespace cadmium {
namespace dynamic {
namespace engine {

template <typename TIME> class simulator : public engine<TIME> {
  using model_type = typename cadmium::dynamic::modeling::atomic_abstract<TIME>;

  std::shared_ptr<cadmium::dynamic::modeling::atomic_abstract<TIME>> _model;
  TIME _last;
  TIME _next;
  cadmium::dynamic::message_bags _outbox;
  cadmium::dynamic::message_bags _inbox;

public:
  simulator() = delete;

  explicit simulator(
      std::shared_ptr<cadmium::dynamic::modeling::atomic_abstract<TIME>> model)
      : _model(model) {}

  void init(TIME initial_time) override {
    cadmium::log::emit(cadmium::log::level::info, "sim_info_init",
                       _model->get_id(),
                       cadmium::log::to_sim_double(initial_time));
    _last = initial_time;
    _next = initial_time + _model->time_advance();
    cadmium::log::emit(cadmium::log::level::debug, "sim_state",
                       _model->get_id() + " " + _model->model_state_as_string(),
                       cadmium::log::to_sim_double(initial_time));
  }

  std::string get_model_id() const override { return _model->get_id(); }

  TIME next() const noexcept override { return _next; }

  void collect_outputs(const TIME &t) override {
    cadmium::log::emit(cadmium::log::level::debug, "sim_info_collect",
                       _model->get_id(), cadmium::log::to_sim_double(t));
    _inbox = cadmium::dynamic::message_bags();
    if (_next < t) {
      throw std::domain_error("Trying to obtain output in a higher time than "
                              "the next scheduled internal event");
    } else if (_next == t) {
      _outbox = _model->output();
      cadmium::log::emit(cadmium::log::level::debug, "sim_messages_collect",
                         _model->get_id() + " " +
                             _model->messages_by_port_as_string(_outbox),
                         cadmium::log::to_sim_double(t));
    } else {
      _outbox = cadmium::dynamic::message_bags();
    }
  }

  cadmium::dynamic::message_bags &outbox() override { return _outbox; }

  cadmium::dynamic::message_bags &inbox() override { return _inbox; }

  void advance_simulation(const TIME &t) override {
    _outbox = cadmium::dynamic::message_bags();

    cadmium::log::emit(cadmium::log::level::debug, "sim_info_advance",
                       _model->get_id(), cadmium::log::to_sim_double(t));

    if (t < _last) {
      throw std::domain_error("Event received for executing in the past of "
                              "current simulation time");
    } else if (_next < t) {
      throw std::domain_error(
          "Event received for executing after next internal event");
    } else {
      if (!_inbox.empty()) {
        if (t == _next) {
          _model->confluence_transition(t - _last, _inbox);
        } else {
          _model->external_transition(t - _last, _inbox);
        }
        _last = t;
        _next = _last + _model->time_advance();
        _inbox = cadmium::dynamic::message_bags();
      } else {
        if (t == _next) {
          _model->internal_transition();
          _last = t;
          _next = _last + _model->time_advance();
        }
      }
    }

    cadmium::log::emit(cadmium::log::level::debug, "sim_state",
                       _model->get_id() + " " + _model->model_state_as_string(),
                       cadmium::log::to_sim_double(t));
  }
};

} // namespace engine
} // namespace dynamic
} // namespace cadmium
