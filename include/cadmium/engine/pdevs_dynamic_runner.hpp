/**
 * Copyright (c) 2018-present, Damian Vicino, Laouen M. L. Belloli, Ezequiel
 * Pecker-Marcosig Carleton University, Universite de Nice-Sophia Antipolis,
 * Universidad de Buenos Aires All rights reserved.
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

#include <cadmium/engine/pdevs_dynamic_coordinator.hpp>
#include <cadmium/engine/pdevs_dynamic_simulator.hpp>
#include <cadmium/logger/cadmium_log.hpp>

#include <iostream>
#include <limits>
#include <memory>

namespace cadmium {
    namespace dynamic {
        namespace engine {

            template <class TIME> class runner {
                TIME _next;
                bool _progress_bar = false;
                std::shared_ptr<cadmium::dynamic::engine::engine<TIME>> _top_engine;

                void init(const TIME &init_time) {
                    cadmium::log::emit(cadmium::log::level::info, "run_global_time", "start",
                                       cadmium::log::to_sim_double(init_time));
                    cadmium::log::emit(cadmium::log::level::info, "run_info", "Preparing model",
                                       cadmium::log::to_sim_double(init_time));
                    _top_engine->init(init_time);
                    _next = _top_engine->next();
                }

              public:
                explicit runner(
                    std::shared_ptr<cadmium::dynamic::modeling::coupled<TIME>> coupled_model,
                    const TIME &init_time)
                    : _top_engine(std::make_shared<cadmium::dynamic::engine::coordinator<TIME>>(
                          coupled_model)) {
                    init(init_time);
                }

                explicit runner(
                    std::shared_ptr<cadmium::dynamic::modeling::atomic_abstract<TIME>> atomic_model,
                    const TIME &init_time)
                    : _top_engine(std::make_shared<cadmium::dynamic::engine::simulator<TIME>>(
                          atomic_model)) {
                    init(init_time);
                }

                TIME run_until(const TIME &t) {
                    cadmium::log::emit(cadmium::log::level::info, "run_info", "Starting run");
                    while (_next < t) {
                        cadmium::log::emit(cadmium::log::level::debug, "run_global_time", "step",
                                           cadmium::log::to_sim_double(_next));
                        _top_engine->collect_outputs(_next);
                        _top_engine->advance_simulation(_next);
                        _next = _top_engine->next();
                        if (_progress_bar)
                            progress_bar_meter(_next, t);
                    }
                    turn_progress_off();
                    if (_next == std::numeric_limits<TIME>::infinity()) {
                        cadmium::log::emit(cadmium::log::level::warn, "run_info",
                                           "simulation passivated before reaching end time");
                    }
                    cadmium::log::emit(cadmium::log::level::info, "run_info", "Finished run");
                    return _next;
                }

                void run_until_passivate() {
                    run_until(std::numeric_limits<TIME>::infinity());
                }

                void turn_progress_on() {
                    _progress_bar = true;
                    std::cout << "\033[33m" << std::flush;
                }

                void turn_progress_off() {
                    _progress_bar = false;
                    std::cout << "\033[0m" << std::flush;
                }

              private:
                void progress_bar_meter(TIME current, TIME total) {
                    std::cout << "\r[" << current << "/";
                    if (total == std::numeric_limits<TIME>::infinity())
                        std::cout << "inf]";
                    else
                        std::cout << total << "]";
                    std::cout << std::flush;
                }
            };

        } // namespace engine
    } // namespace dynamic
} // namespace cadmium
