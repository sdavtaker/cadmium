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

#ifndef CADMIUM_STDEVS_RUNNER_HPP
#define CADMIUM_STDEVS_RUNNER_HPP

#include <cadmium/concepts/stdevs_concepts.hpp>
#include <cadmium/engine/stdevs_coordinator.hpp>
#include <cadmium/engine/stdevs_engine_helpers.hpp>
#include <cadmium/engine/stdevs_simulator.hpp>
#include <cadmium/logger/cadmium_log.hpp>

#include <limits>
#include <random>

namespace cadmium {
    namespace engine {
        namespace stdevs {

            /**
             * STDEVS root coordinator (runner).
             *
             * Owns the URNG and seeds it at construction time.  The seed is logged so
             * that any simulation can be reproduced exactly by passing the same seed.
             *
             * Accepts either a coupled model (coordinator engine) or an atomic model
             * (simulator engine).  The inner loop drains all imminent models at the
             * same logical time before advancing the clock, identical to devs::runner.
             *
             * @tparam TIME    Simulation time type.
             * @tparam MODEL   Model template (coupled or atomic).
             * @tparam URNG    Uniform random bit generator (default std::mt19937).
             */
            template <class TIME, template <class> class MODEL, typename URNG = std::mt19937>
                requires cadmium::concepts::Time<TIME> &&
                         (cadmium::concepts::stdevs::CoupledModel<MODEL<TIME>> ||
                          cadmium::concepts::stdevs::AtomicModel<MODEL<TIME>, TIME, URNG>)
            class runner {
                using engine_type = typename stdevs_select_engine_type<
                    cadmium::concepts::stdevs::AtomicModel<MODEL<TIME>, TIME, URNG>, MODEL, TIME,
                    URNG>::type;

                URNG _rng;
                TIME _next{};
                engine_type _top_engine;

              public:
                // Seeded constructor — fully reproducible; seed is logged.
                explicit runner(const TIME &init_time, typename URNG::result_type seed)
                    : _rng(seed) {
                    cadmium::log::emit(cadmium::log::level::info, "stdev_run_global_time", "start",
                                       cadmium::log::to_sim_double(init_time));
                    cadmium::log::emit(cadmium::log::level::info, "stdev_run_rng_seed",
                                       std::to_string(seed));
                    cadmium::log::emit(cadmium::log::level::info, "stdev_run_info",
                                       "Preparing model", cadmium::log::to_sim_double(init_time));
                    _top_engine.init(init_time, _rng);
                    _next = _top_engine.next();
                }

                // Non-deterministic constructor — uses std::random_device.
                explicit runner(const TIME &init_time)
                    : runner(init_time,
                             static_cast<typename URNG::result_type>(std::random_device{}())) {}

                TIME run_until(const TIME &t) {
                    cadmium::log::emit(cadmium::log::level::info, "stdev_run_info", "Starting run");
                    while (_next < t) {
                        const TIME t_current = _next;
                        cadmium::log::emit(cadmium::log::level::debug, "stdev_run_global_time",
                                           "step", cadmium::log::to_sim_double(t_current));
                        while (_next == t_current) {
                            _top_engine.collect_outputs(t_current);
                            _top_engine.advance_simulation(t_current);
                            _next = _top_engine.next();
                        }
                    }
                    if (_next == std::numeric_limits<TIME>::infinity()) {
                        cadmium::log::emit(cadmium::log::level::warn, "stdev_run_info",
                                           "simulation passivated before reaching end time");
                    }
                    cadmium::log::emit(cadmium::log::level::info, "stdev_run_info", "Finished run");
                    return _next;
                }

                void run_until_passivate() {
                    run_until(std::numeric_limits<TIME>::infinity());
                }
            };

        } // namespace stdevs
    } // namespace engine
} // namespace cadmium

#endif // CADMIUM_STDEVS_RUNNER_HPP
