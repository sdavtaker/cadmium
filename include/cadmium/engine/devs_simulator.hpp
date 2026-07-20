// SPDX-License-Identifier: BSD-2-Clause
/**
 * Copyright (c) 2026-present, Damian Vicino
 * Carleton University
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

#ifndef CADMIUM_DEVS_SIMULATOR_HPP
#define CADMIUM_DEVS_SIMULATOR_HPP

#include <cadmium/concepts/devs_concepts.hpp>
#include <cadmium/engine/devs_engine_helpers.hpp>
#include <cadmium/logger/cadmium_log.hpp>
#include <cadmium/logger/common_loggers_helpers.hpp>
#include <cadmium/modeling/message_box.hpp>

#include <optional>
#include <sstream>
#include <stdexcept>

namespace cadmium {
    namespace engine {
        namespace devs {

            template <template <typename T> class MODEL, typename TIME>
                requires cadmium::concepts::devs::AtomicModel<MODEL<TIME>, TIME>
            class simulator {

                using input_ports  = typename MODEL<TIME>::input_ports;
                using output_ports = typename MODEL<TIME>::output_ports;
                using in_box_type  = typename make_message_box<input_ports>::type;
                using out_box_type = typename make_message_box<output_ports>::type;

                MODEL<TIME> _model;
                TIME _last{};
                TIME _next{};
                std::string _model_id{};
                std::string _id_suffix{};
                in_box_type _inbox{};
                out_box_type _outbox{};

              public:
                using model_type = MODEL<TIME>;

                /**
                 * Append a disambiguating suffix to the model id used in log
                 * lines (e.g. "[3]" for pack elements). Must be set before
                 * init().
                 */
                void set_model_id_suffix(std::string suffix) {
                    _id_suffix = std::move(suffix);
                }

                void init(TIME initial_time) {
                    _model_id = cadmium::logger::model_type_name<model_type>() + _id_suffix;
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

                TIME next() const noexcept {
                    return _next;
                }

                void collect_outputs(const TIME &t) {
                    cadmium::log::emit(cadmium::log::level::debug, "sim_info_collect", _model_id,
                                       cadmium::log::to_sim_double(t));

                    _inbox = in_box_type{};

                    if (_next < t) {
                        cadmium::log::emit(cadmium::log::level::error, "sim_error",
                                           _model_id + ": collect_outputs called past next event",
                                           cadmium::log::to_sim_double(t));
                        throw std::domain_error(
                            "Trying to obtain output when no internal event is scheduled");
                    } else if (_next == t) {
                        _outbox = _model.output();
                    } else {
                        _outbox = out_box_type{};
                    }

                    std::ostringstream oss;
                    print_box_by_port(oss, _outbox);
                    cadmium::log::emit(cadmium::log::level::debug, "sim_messages_collect",
                                       _model_id + " " + oss.str(), cadmium::log::to_sim_double(t));
                }

                const out_box_type &outbox() const noexcept {
                    return _outbox;
                }

                void inbox(in_box_type in) noexcept {
                    _inbox = std::move(in);
                }

                bool inbox_empty() const noexcept {
                    return all_box_empty(_inbox);
                }

                template <typename PORT>
                const std::optional<typename PORT::message_type> &outbox_port() const noexcept {
                    return cadmium::get_message<PORT>(_outbox);
                }

                // Used by the coordinator to route one message per port into the inbox.
                template <typename PORT>
                void set_inbox_port(std::optional<typename PORT::message_type> msg) {
                    cadmium::get_message<PORT>(_inbox) = std::move(msg);
                }

                void advance_simulation(TIME t) {
                    _outbox = out_box_type{};

                    cadmium::log::emit(cadmium::log::level::debug, "sim_info_advance", _model_id,
                                       cadmium::log::to_sim_double(t));

                    if (t < _last) {
                        cadmium::log::emit(cadmium::log::level::error, "sim_error",
                                           _model_id + ": advance_simulation called in the past",
                                           cadmium::log::to_sim_double(t));
                        throw std::domain_error(
                            "Event received for executing in the past of current simulation time");
                    } else if (_next < t) {
                        cadmium::log::emit(
                            cadmium::log::level::error, "sim_error",
                            _model_id + ": advance_simulation called past next scheduled event",
                            cadmium::log::to_sim_double(t));
                        throw std::domain_error(
                            "Event received for executing after next internal event");
                    } else {
                        if (!all_box_empty(_inbox)) {
                            // Classic DEVS: external input → external_transition only.
                            // No confluence_transition; SELECT at coordinator level serialises
                            // simultaneous internal and external events.
                            _model.external_transition(t - _last, _inbox);
                            _last  = t;
                            _next  = _last + _model.time_advance();
                            _inbox = in_box_type{};
                        } else if (t == _next) {
                            _model.internal_transition();
                            _last = t;
                            _next = _last + _model.time_advance();
                        }
                        // t != _next with empty inbox: no-op (simulator not selected by
                        // coordinator)
                    }

                    std::ostringstream oss;
                    oss << _model.state;
                    cadmium::log::emit(cadmium::log::level::debug, "sim_state",
                                       _model_id + " " + oss.str(), cadmium::log::to_sim_double(t));
                }
            };

        } // namespace devs
    } // namespace engine
} // namespace cadmium

#endif // CADMIUM_DEVS_SIMULATOR_HPP
