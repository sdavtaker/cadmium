/**
 * Copyright (c) 2018-present, Laouen M. L. Belloli, Damian Vicino
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
#include <cadmium/engine/pdevs_dynamic_engine_helpers.hpp>
#include <cadmium/engine/pdevs_dynamic_simulator.hpp>
#include <cadmium/logger/cadmium_log.hpp>
#include <cadmium/modeling/dynamic_coupled.hpp>
#include <cadmium/modeling/dynamic_message_bag.hpp>

#include <limits>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace cadmium {
    namespace dynamic {
        namespace engine {

            template <typename TIME>
            class coordinator : public cadmium::dynamic::engine::engine<TIME> {

                TIME _last;
                TIME _next;

                std::string _model_id;

                subcoordinators_type<TIME> _subcoordinators;
                external_couplings<TIME> _external_output_couplings;
                external_couplings<TIME> _external_input_couplings;
                internal_couplings<TIME> _internal_coupligns;
                dynamic::message_bags _inbox;
                dynamic::message_bags _outbox;

              public:
                using model_type = typename cadmium::dynamic::modeling::coupled<TIME>;

                coordinator() = delete;

                explicit coordinator(std::shared_ptr<model_type> coupled_model)
                    : _model_id(coupled_model->get_id()) {
                    std::map<std::string, std::shared_ptr<engine<TIME>>> engines_by_id;

                    for (auto &m : coupled_model->_models) {
                        std::shared_ptr<cadmium::dynamic::modeling::coupled<TIME>> m_coupled =
                            std::dynamic_pointer_cast<cadmium::dynamic::modeling::coupled<TIME>>(m);
                        std::shared_ptr<cadmium::dynamic::modeling::atomic_abstract<TIME>>
                            m_atomic = std::dynamic_pointer_cast<
                                cadmium::dynamic::modeling::atomic_abstract<TIME>>(m);

                        if (m_coupled == nullptr) {
                            if (m_atomic == nullptr) {
                                cadmium::log::emit(cadmium::log::level::error, "coor_error",
                                                   _model_id +
                                                       ": submodel is neither coupled nor atomic");
                                throw std::domain_error(
                                    "Invalid submodel is neither coupled nor atomic");
                            }
                            _subcoordinators.push_back(
                                std::make_shared<cadmium::dynamic::engine::simulator<TIME>>(
                                    m_atomic));
                        } else {
                            if (m_atomic != nullptr) {
                                cadmium::log::emit(cadmium::log::level::error, "coor_error",
                                                   _model_id +
                                                       ": submodel is both coupled and atomic");
                                throw std::domain_error(
                                    "Invalid submodel is defined as both coupled and atomic");
                            }
                            _subcoordinators.push_back(
                                std::make_shared<cadmium::dynamic::engine::coordinator<TIME>>(
                                    m_coupled));
                        }

                        engines_by_id.insert(std::make_pair(_subcoordinators.back()->get_model_id(),
                                                            _subcoordinators.back()));
                    }

                    for (const auto &eoc : coupled_model->_eoc) {
                        if (engines_by_id.find(eoc._from) == engines_by_id.end()) {
                            cadmium::log::emit(cadmium::log::level::error, "coor_error",
                                               _model_id + ": EOC from unknown model " + eoc._from);
                            throw std::domain_error("External output coupling from invalid model");
                        }
                        cadmium::dynamic::engine::external_coupling<TIME> new_eoc;
                        new_eoc.first = engines_by_id.at(eoc._from);
                        new_eoc.second.push_back(eoc._link);
                        _external_output_couplings.push_back(new_eoc);
                    }

                    for (const auto &eic : coupled_model->_eic) {
                        if (engines_by_id.find(eic._to) == engines_by_id.end()) {
                            cadmium::log::emit(cadmium::log::level::error, "coor_error",
                                               _model_id + ": EIC to unknown model " + eic._to);
                            throw std::domain_error("External input coupling to invalid model");
                        }
                        cadmium::dynamic::engine::external_coupling<TIME> new_eic;
                        new_eic.first = engines_by_id.at(eic._to);
                        new_eic.second.push_back(eic._link);
                        _external_input_couplings.push_back(new_eic);
                    }

                    for (const auto &ic : coupled_model->_ic) {
                        if (engines_by_id.find(ic._from) == engines_by_id.end() ||
                            engines_by_id.find(ic._to) == engines_by_id.end()) {
                            cadmium::log::emit(cadmium::log::level::error, "coor_error",
                                               _model_id + ": IC references unknown model(s) " +
                                                   ic._from + " -> " + ic._to);
                            throw std::domain_error("Internal coupling to invalid model");
                        }
                        cadmium::dynamic::engine::internal_coupling<TIME> new_ic;
                        new_ic.first.first  = engines_by_id.at(ic._from);
                        new_ic.first.second = engines_by_id.at(ic._to);
                        new_ic.second.push_back(ic._link);
                        _internal_coupligns.push_back(new_ic);
                    }
                }

                void init(TIME initial_time) override {
                    cadmium::log::emit(cadmium::log::level::info, "coor_info_init", _model_id,
                                       cadmium::log::to_sim_double(initial_time));
                    _last = initial_time;
                    cadmium::dynamic::engine::init_subcoordinators<TIME>(initial_time,
                                                                         _subcoordinators);
                    _next = cadmium::dynamic::engine::min_next_in_subcoordinators<TIME>(
                        _subcoordinators);
                }

                std::string get_model_id() const override {
                    return _model_id;
                }

                TIME next() const noexcept override {
                    return _next;
                }

                void collect_outputs(const TIME &t) override {
                    cadmium::log::emit(cadmium::log::level::debug, "coor_info_collect", _model_id,
                                       cadmium::log::to_sim_double(t));

                    if (_next < t) {
                        cadmium::log::emit(cadmium::log::level::error, "coor_error",
                                           _model_id + ": collect_outputs called past next event",
                                           cadmium::log::to_sim_double(t));
                        throw std::domain_error(
                            "Trying to obtain output when not internal event is scheduled");
                    } else if (_next == t) {
                        cadmium::log::emit(cadmium::log::level::debug, "coor_routing_eoc_collect",
                                           _model_id, cadmium::log::to_sim_double(t));
                        cadmium::dynamic::engine::collect_outputs_in_subcoordinators<TIME>(
                            t, _subcoordinators);
                        _outbox = cadmium::dynamic::engine::collect_messages_by_eoc<TIME>(
                            _external_output_couplings);
                    }
                }

                cadmium::dynamic::message_bags &outbox() override {
                    return _outbox;
                }

                cadmium::dynamic::message_bags &inbox() override {
                    return _inbox;
                }

                void advance_simulation(const TIME &t) override {
                    _outbox = cadmium::dynamic::message_bags();

                    cadmium::log::emit(cadmium::log::level::debug, "coor_info_advance", _model_id,
                                       cadmium::log::to_sim_double(t));

                    if (_next < t || t < _last) {
                        cadmium::log::emit(cadmium::log::level::error, "coor_error",
                                           _model_id +
                                               ": advance_simulation called outside time scope",
                                           cadmium::log::to_sim_double(t));
                        throw std::domain_error(
                            "Trying to obtain output when out of the advance time scope");
                    }

                    cadmium::log::emit(cadmium::log::level::debug, "coor_routing_ic_collect",
                                       _model_id, cadmium::log::to_sim_double(t));
                    cadmium::dynamic::engine::route_internal_coupled_messages_on_subcoordinators<
                        TIME>(_internal_coupligns);

                    cadmium::log::emit(cadmium::log::level::debug, "coor_routing_eic_collect",
                                       _model_id, cadmium::log::to_sim_double(t));
                    cadmium::dynamic::engine::
                        route_external_input_coupled_messages_on_subcoordinators<TIME>(
                            _inbox, _external_input_couplings);

                    cadmium::dynamic::engine::advance_simulation_in_subengines<TIME>(
                        t, _subcoordinators);

                    _last = t;
                    _next = cadmium::dynamic::engine::min_next_in_subcoordinators<TIME>(
                        _subcoordinators);
                    _inbox = cadmium::dynamic::message_bags();
                }
            };

        } // namespace engine
    } // namespace dynamic
} // namespace cadmium
