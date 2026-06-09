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

#ifndef CADMIUM_STDEVS_SIMULATOR_HPP
#define CADMIUM_STDEVS_SIMULATOR_HPP

#include <cadmium/concepts/stdevs_concepts.hpp>
#include <cadmium/engine/stdevs_engine_helpers.hpp>
#include <cadmium/logger/cadmium_log.hpp>
#include <cadmium/logger/common_loggers_helpers.hpp>
#include <cadmium/modeling/message_box.hpp>

#include <optional>
#include <sstream>
#include <stdexcept>

namespace cadmium {
    namespace engine {
        namespace stdevs {

            /**
             * STDEVS atomic simulator.
             *
             * Mirrors devs::simulator exactly except:
             *  - init(TIME, URNG&) stores a pointer to the RNG injected by the runner.
             *  - advance_simulation(TIME) passes *_rng to both internal_transition and
             *    external_transition, satisfying the STDEVS Activity-c interface (CKW10
             *    Fig. 3 / Corollary 1).
             *  - collect_outputs and time_advance are deterministic — no RNG needed.
             */
            template <template <typename T> class MODEL, typename TIME, typename URNG>
                requires cadmium::concepts::stdevs::AtomicModel<MODEL<TIME>, TIME, URNG>
            class simulator {

                using input_ports  = typename MODEL<TIME>::input_ports;
                using output_ports = typename MODEL<TIME>::output_ports;
                using in_box_type  = typename make_message_box<input_ports>::type;
                using out_box_type = typename make_message_box<output_ports>::type;

                MODEL<TIME> _model;
                TIME _last{};
                TIME _next{};
                std::string _model_id{};
                in_box_type _inbox{};
                out_box_type _outbox{};
                URNG *_rng = nullptr; // injected at init time — never null after init

              public:
                using model_type = MODEL<TIME>;

                void init(TIME initial_time, URNG &rng) {
                    _rng      = &rng;
                    _model_id = cadmium::logger::model_type_name<model_type>();
                    cadmium::log::emit(cadmium::log::level::info, "stdev_sim_info_init", _model_id,
                                       cadmium::log::to_sim_double(initial_time));

                    _last = initial_time;
                    _next = initial_time + _model.time_advance();

                    std::ostringstream oss;
                    cadmium::logger::print_value_or_name(oss, _model.state);
                    cadmium::log::emit(cadmium::log::level::debug, "stdev_sim_state",
                                       _model_id + " " + oss.str(),
                                       cadmium::log::to_sim_double(initial_time));
                }

                TIME next() const noexcept {
                    return _next;
                }

                void collect_outputs(const TIME &t) {
                    cadmium::log::emit(cadmium::log::level::debug, "stdev_sim_info_collect",
                                       _model_id, cadmium::log::to_sim_double(t));

                    _inbox = in_box_type{};

                    if (_next < t) {
                        cadmium::log::emit(cadmium::log::level::error, "stdev_sim_error",
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
                    cadmium::engine::devs::print_box_by_port(oss, _outbox);
                    cadmium::log::emit(cadmium::log::level::debug, "stdev_sim_messages_collect",
                                       _model_id + " " + oss.str(), cadmium::log::to_sim_double(t));
                }

                const out_box_type &outbox() const noexcept {
                    return _outbox;
                }

                void inbox(in_box_type in) noexcept {
                    _inbox = std::move(in);
                }

                bool inbox_empty() const noexcept {
                    return cadmium::engine::devs::all_box_empty(_inbox);
                }

                template <typename PORT>
                const std::optional<typename PORT::message_type> &outbox_port() const noexcept {
                    return cadmium::get_message<PORT>(_outbox);
                }

                template <typename PORT>
                void set_inbox_port(std::optional<typename PORT::message_type> msg) {
                    cadmium::get_message<PORT>(_inbox) = std::move(msg);
                }

                void advance_simulation(TIME t) {
                    _outbox = out_box_type{};

                    cadmium::log::emit(cadmium::log::level::debug, "stdev_sim_info_advance",
                                       _model_id, cadmium::log::to_sim_double(t));

                    if (t < _last) {
                        cadmium::log::emit(cadmium::log::level::error, "stdev_sim_error",
                                           _model_id + ": advance_simulation called in the past",
                                           cadmium::log::to_sim_double(t));
                        throw std::domain_error(
                            "Event received for executing in the past of current simulation time");
                    } else if (_next < t) {
                        cadmium::log::emit(
                            cadmium::log::level::error, "stdev_sim_error",
                            _model_id + ": advance_simulation called past next scheduled event",
                            cadmium::log::to_sim_double(t));
                        throw std::domain_error(
                            "Event received for executing after next internal event");
                    } else {
                        if (!cadmium::engine::devs::all_box_empty(_inbox)) {
                            _model.external_transition(t - _last, _inbox, *_rng);
                            _last  = t;
                            _next  = _last + _model.time_advance();
                            _inbox = in_box_type{};
                        } else if (t == _next) {
                            _model.internal_transition(*_rng);
                            _last = t;
                            _next = _last + _model.time_advance();
                        }
                    }

                    std::ostringstream oss;
                    cadmium::logger::print_value_or_name(oss, _model.state);
                    cadmium::log::emit(cadmium::log::level::debug, "stdev_sim_state",
                                       _model_id + " " + oss.str(), cadmium::log::to_sim_double(t));
                }
            };

        } // namespace stdevs
    } // namespace engine
} // namespace cadmium

#endif // CADMIUM_STDEVS_SIMULATOR_HPP
