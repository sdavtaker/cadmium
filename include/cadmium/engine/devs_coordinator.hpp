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

#ifndef CADMIUM_DEVS_COORDINATOR_HPP
#define CADMIUM_DEVS_COORDINATOR_HPP

#include <cadmium/concepts/devs_concepts.hpp>
#include <cadmium/engine/devs_engine_helpers.hpp>
#include <cadmium/engine/devs_pack_engine.hpp>
#include <cadmium/engine/devs_simulator.hpp>
#include <cadmium/logger/cadmium_log.hpp>
#include <cadmium/logger/common_loggers_helpers.hpp>
#include <cadmium/modeling/coupling.hpp>
#include <cadmium/modeling/message_box.hpp>
#include <cadmium/modeling/pack.hpp>

#include <limits>
#include <optional>
#include <sstream>
#include <stdexcept>

namespace cadmium {
    namespace engine {
        namespace devs {

            /**
             * Classic DEVS coordinator.
             *
             * Each advance_simulation call fires exactly ONE selected model (internal
             * transition) plus any submodels that receive IC/EIC messages from it
             * (external transition).  The coupled model's SELECT type chooses the
             * imminent model; the default first_imminent picks the lowest tuple index.
             *
             * External input (non-empty _inbox) routes through EIC and fires the
             * receivers only — no SELECT is needed for that case.
             */
            template <template <typename T> class MODEL, typename TIME>
                requires cadmium::concepts::devs::CoupledModel<MODEL<TIME>>
            class coordinator {

                template <typename P>
                using submodels_type = typename MODEL<TIME>::template models<P>;
                using in_box_type =
                    typename make_message_box<typename MODEL<TIME>::input_ports>::type;
                using out_box_type =
                    typename make_message_box<typename MODEL<TIME>::output_ports>::type;
                using subcoordinators_type =
                    typename devs_coordinate_tuple<TIME, submodels_type>::type;
                using eic = typename MODEL<TIME>::external_input_couplings;
                using eoc = typename MODEL<TIME>::external_output_couplings;
                using ic  = typename MODEL<TIME>::internal_couplings;
                using sel = typename MODEL<TIME>::select;

                TIME _last{};
                TIME _next{};
                std::size_t _selected{std::numeric_limits<std::size_t>::max()};
                subcoordinators_type _subcoordinators;
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

                void init(TIME t) {
                    _model_id = cadmium::logger::model_type_name<MODEL<TIME>>() + _id_suffix;
                    cadmium::log::emit(cadmium::log::level::info, "coor_info_init", _model_id,
                                       cadmium::log::to_sim_double(t));

                    _last = t;
                    init_subcoordinators<TIME, subcoordinators_type>(t, _subcoordinators);
                    _next = min_next_in_tuple<subcoordinators_type>(_subcoordinators);
                }

                TIME next() const noexcept {
                    return _next;
                }

                void collect_outputs(const TIME &t) {
                    cadmium::log::emit(cadmium::log::level::debug, "coor_info_collect", _model_id,
                                       cadmium::log::to_sim_double(t));

                    if (_next < t) {
                        cadmium::log::emit(cadmium::log::level::error, "coor_error",
                                           _model_id + ": collect_outputs called past next event",
                                           cadmium::log::to_sim_double(t));
                        throw std::domain_error(
                            "Trying to obtain output when no internal event is scheduled");
                    }

                    if (_next == t) {
                        // SELECT the one imminent model, collect its output, route EOC.
                        _selected = sel::apply(_subcoordinators, t);

                        cadmium::log::emit(cadmium::log::level::debug, "coor_routing_eoc_collect",
                                           _model_id, cadmium::log::to_sim_double(t));

                        collect_selected_output<TIME, subcoordinators_type>(t, _subcoordinators,
                                                                            _selected);
                        _outbox =
                            collect_messages_by_eoc_devs<TIME, eoc, out_box_type,
                                                         subcoordinators_type>(_subcoordinators);
                    } else {
                        _outbox = out_box_type{};
                    }
                }

                const out_box_type &outbox() const noexcept {
                    return _outbox;
                }

                template <typename PORT>
                const std::optional<typename PORT::message_type> &outbox_port() const noexcept {
                    return cadmium::get_message<PORT>(_outbox);
                }

                void inbox(in_box_type in) noexcept {
                    _inbox = std::move(in);
                }

                bool inbox_empty() const noexcept {
                    return all_box_empty(_inbox);
                }

                template <typename PORT>
                void set_inbox_port(std::optional<typename PORT::message_type> msg) {
                    cadmium::get_message<PORT>(_inbox) = std::move(msg);
                }

                void advance_simulation(const TIME &t) {
                    _outbox = out_box_type{};

                    cadmium::log::emit(cadmium::log::level::debug, "coor_info_advance", _model_id,
                                       cadmium::log::to_sim_double(t));

                    if (t < _last || _next < t) {
                        cadmium::log::emit(cadmium::log::level::error, "coor_error",
                                           _model_id +
                                               ": advance_simulation called outside time scope",
                                           cadmium::log::to_sim_double(t));
                        throw std::domain_error(
                            "Trying to advance simulation outside the valid time scope");
                    }

                    if (!all_box_empty(_inbox)) {
                        // External input: route EIC, fire receivers, no SELECT.
                        cadmium::log::emit(cadmium::log::level::debug, "coor_routing_eic_collect",
                                           _model_id, cadmium::log::to_sim_double(t));

                        route_external_input_coupled_messages_devs<TIME, in_box_type,
                                                                   subcoordinators_type, eic>(
                            t, _inbox, _subcoordinators);

                        advance_receivers_only<TIME, subcoordinators_type>(t, _subcoordinators);
                    } else {
                        // Internal event: route IC from selected model, fire selected + receivers.
                        cadmium::log::emit(cadmium::log::level::debug, "coor_routing_ic_collect",
                                           _model_id, cadmium::log::to_sim_double(t));

                        route_internal_coupled_messages_devs<TIME, subcoordinators_type, ic>(
                            t, _subcoordinators);

                        advance_selected_and_receivers<TIME, subcoordinators_type>(
                            t, _subcoordinators, _selected);
                    }

                    _last  = t;
                    _next  = min_next_in_tuple<subcoordinators_type>(_subcoordinators);
                    _inbox = in_box_type{};
                }
            };

        } // namespace devs
    } // namespace engine
} // namespace cadmium

#endif // CADMIUM_DEVS_COORDINATOR_HPP
