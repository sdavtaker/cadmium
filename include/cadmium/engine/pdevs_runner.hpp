// SPDX-License-Identifier: BSD-2-Clause
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

#ifndef CADMIUM_PDEVS_RUNNER_HPP
#define CADMIUM_PDEVS_RUNNER_HPP

#include <cadmium/concepts/pdevs_concepts.hpp>
#include <cadmium/engine/pdevs_coordinator.hpp>
#include <cadmium/engine/pdevs_engine_helpers.hpp>
#include <cadmium/engine/pdevs_simulator.hpp>
#include <cadmium/logger/cadmium_log.hpp>

namespace cadmium {
    namespace engine {

        template <class TIME, template <class> class MODEL>
            requires cadmium::concepts::Time<TIME> &&
                     (cadmium::concepts::pdevs::CoupledModel<MODEL<TIME>> ||
                      cadmium::concepts::pdevs::AtomicModel<MODEL<TIME>, TIME>)
        class runner {
            using engine_type = typename cadmium::engine::select_engine_type<
                cadmium::concepts::pdevs::AtomicModel<MODEL<TIME>, TIME>, MODEL, TIME>::type;

            TIME _next{};
            engine_type _top_engine;

          public:
            explicit runner(const TIME &init_time) {
                cadmium::log::emit(cadmium::log::level::info, "run_global_time", "start",
                                   cadmium::log::to_sim_double(init_time));
                cadmium::log::emit(cadmium::log::level::info, "run_info", "Preparing model",
                                   cadmium::log::to_sim_double(init_time));
                _top_engine.init(init_time);
                _next = _top_engine.next();
            }

            TIME run_until(const TIME &t) {
                cadmium::log::emit(cadmium::log::level::info, "run_info", "Starting run");
                while (_next < t) {
                    cadmium::log::emit(cadmium::log::level::debug, "run_global_time", "step",
                                       cadmium::log::to_sim_double(_next));
                    _top_engine.collect_outputs(_next);
                    _top_engine.advance_simulation(_next);
                    _next = _top_engine.next();
                }
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
        };

    } // namespace engine
} // namespace cadmium

#endif // CADMIUM_PDEVS_RUNNER_HPP
