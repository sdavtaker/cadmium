/**
 * Copyright (c) 2018-present, Laouen M. L. Belloli
 * Carleton University, Universidad de Buenos Aires
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

#ifndef CADMIUM_DYNAMIC_COMMON_LOGGERS_HPP
#define CADMIUM_DYNAMIC_COMMON_LOGGERS_HPP

#include <cadmium/engine/common_helpers.hpp>
#include <cadmium/logger/cadmium_log.hpp>
#include <sstream>
#include <vector>
#include <string>

namespace cadmium {
    namespace dynamic {
        namespace logger {

            struct routed_messages {
                std::string from_port;
                std::string to_port;
                std::vector<std::string> from_messages;
                std::vector<std::string> to_messages;

                routed_messages() = default;

                routed_messages(
                        std::string from_p,
                        std::string to_p
                ) :
                        from_port(from_p),
                        to_port(to_p)
                {}

                routed_messages(
                        std::vector<std::string> from_msgs,
                        std::vector<std::string> to_msgs,
                        std::string from_p,
                        std::string to_p
                ) :
                        from_port(from_p),
                        to_port(to_p),
                        from_messages(from_msgs),
                        to_messages(to_msgs)
                {}

                routed_messages(const routed_messages& other) :
                        from_port(other.from_port),
                        to_port(other.to_port),
                        from_messages(other.from_messages),
                        to_messages(other.to_messages)
                {}

            };

            // ── Ostream-based formatter (human-readable, for tests and debugging) ──────

            template<typename TIME>
            struct formatter {

                template<typename SINK>
                static void coor_info_init(const TIME& t, const std::string& model_id) {
                    SINK::sink() << "Coordinator for model " << model_id
                                 << " initialized to time " << t << "\n";
                }

                template<typename SINK>
                static void coor_info_collect(const TIME& t, const std::string& model_id) {
                    SINK::sink() << "Coordinator for model " << model_id
                                 << " collecting output at time " << t << "\n";
                }

                template<typename SINK>
                static void coor_routing_eoc_collect(const TIME&, const std::string& model_id) {
                    SINK::sink() << "EOC for model " << model_id << "\n";
                }

                template<typename SINK>
                static void coor_info_advance(const TIME& from, const TIME& to,
                                              const std::string& model_id) {
                    SINK::sink() << "Coordinator for model " << model_id
                                 << " advancing simulation from time " << from
                                 << " to " << to << "\n";
                }

                template<typename SINK>
                static void coor_routing_ic_collect(const TIME&, const std::string& model_id) {
                    SINK::sink() << "IC for model " << model_id << "\n";
                }

                template<typename SINK>
                static void coor_routing_eic_collect(const TIME&, const std::string& model_id) {
                    SINK::sink() << "EIC for model " << model_id << "\n";
                }

                template<typename SINK>
                static void coor_routing_collect(const std::string& from_port,
                                                 const std::string& to_port,
                                                 const std::vector<std::string>& from_messages,
                                                 const std::vector<std::string>& to_messages) {
                    SINK::sink() << " in port " << to_port
                                 << " has " << cadmium::helper::join(to_messages)
                                 << " routed from " << from_port
                                 << " with messages " << cadmium::helper::join(from_messages) << "\n";
                }

                template<typename SINK>
                static void sim_info_init(const TIME& t, const std::string& model_id) {
                    SINK::sink() << "Simulator for model " << model_id
                                 << " initialized to time " << t << "\n";
                }

                template<typename SINK>
                static void sim_state(const TIME&, const std::string& model_id,
                                      const std::string& model_state) {
                    SINK::sink() << "State for model " << model_id
                                 << " is " << model_state << "\n";
                }

                template<typename SINK>
                static void sim_info_collect(const TIME& t, const std::string& model_id) {
                    SINK::sink() << "Simulator for model " << model_id
                                 << " collecting output at time " << t << "\n";
                }

                template<typename SINK>
                static void sim_messages_collect(const TIME&, const std::string& model_id,
                                                 const std::string& outbox) {
                    SINK::sink() << outbox << " generated by model " << model_id << "\n";
                }

                template<typename SINK>
                static void sim_info_advance(const TIME& from, const TIME& to,
                                             const std::string& model_id) {
                    SINK::sink() << "Simulator for model " << model_id
                                 << " advancing simulation from time " << from
                                 << " to " << to << "\n";
                }

                template<typename SINK>
                static void sim_local_time(const TIME& from, const TIME& to,
                                           const std::string& model_id) {
                    SINK::sink() << "Elapsed in model " << model_id
                                 << " is " << (to - from) << "s\n";
                }

                template<typename SINK>
                static void run_global_time(const TIME& t) {
                    SINK::sink() << t << "\n";
                }

                template<typename SINK>
                static void run_info(const std::string& message) {
                    SINK::sink() << message << "\n";
                }
            };

            // ── NDJSON formatter — emits via cadmium::log, SINK is ignored ────────────

            template<typename TIME>
            struct ndjson_formatter {

                template<typename SINK>
                static void coor_info_init(const TIME& t, const std::string& model_id) {
                    cadmium::log::emit(cadmium::log::level::debug, "coordinator_init",
                                       "model=" + model_id,
                                       cadmium::log::to_sim_double(t));
                }

                template<typename SINK>
                static void coor_info_collect(const TIME& t, const std::string& model_id) {
                    cadmium::log::emit(cadmium::log::level::debug, "coordinator_collect",
                                       "model=" + model_id,
                                       cadmium::log::to_sim_double(t));
                }

                template<typename SINK>
                static void coor_routing_eoc_collect(const TIME&, const std::string& model_id) {
                    cadmium::log::emit(cadmium::log::level::debug, "routing_eoc",
                                       "model=" + model_id);
                }

                template<typename SINK>
                static void coor_info_advance(const TIME&, const TIME& to,
                                              const std::string& model_id) {
                    cadmium::log::emit(cadmium::log::level::debug, "coordinator_advance",
                                       "model=" + model_id,
                                       cadmium::log::to_sim_double(to));
                }

                template<typename SINK>
                static void coor_routing_ic_collect(const TIME&, const std::string& model_id) {
                    cadmium::log::emit(cadmium::log::level::debug, "routing_ic",
                                       "model=" + model_id);
                }

                template<typename SINK>
                static void coor_routing_eic_collect(const TIME&, const std::string& model_id) {
                    cadmium::log::emit(cadmium::log::level::debug, "routing_eic",
                                       "model=" + model_id);
                }

                template<typename SINK>
                static void coor_routing_collect(const std::string& from_port,
                                                 const std::string& to_port,
                                                 const std::vector<std::string>& from_messages,
                                                 const std::vector<std::string>& to_messages) {
                    cadmium::log::emit(cadmium::log::level::debug, "routing_messages",
                                       "from=" + from_port + " to=" + to_port
                                       + " msgs=" + cadmium::helper::join(from_messages));
                }

                template<typename SINK>
                static void sim_info_init(const TIME& t, const std::string& model_id) {
                    cadmium::log::emit(cadmium::log::level::debug, "simulator_init",
                                       "model=" + model_id,
                                       cadmium::log::to_sim_double(t));
                }

                template<typename SINK>
                static void sim_state(const TIME&, const std::string& model_id,
                                      const std::string& state) {
                    cadmium::log::emit(cadmium::log::level::debug, "simulator_state",
                                       "model=" + model_id + " state=" + state);
                }

                template<typename SINK>
                static void sim_info_collect(const TIME& t, const std::string& model_id) {
                    cadmium::log::emit(cadmium::log::level::debug, "simulator_collect",
                                       "model=" + model_id,
                                       cadmium::log::to_sim_double(t));
                }

                template<typename SINK>
                static void sim_messages_collect(const TIME&, const std::string& model_id,
                                                 const std::string& messages_by_port) {
                    cadmium::log::emit(cadmium::log::level::debug, "simulator_messages",
                                       "model=" + model_id + " " + messages_by_port);
                }

                template<typename SINK>
                static void sim_info_advance(const TIME&, const TIME& to,
                                             const std::string& model_id) {
                    cadmium::log::emit(cadmium::log::level::debug, "simulator_advance",
                                       "model=" + model_id,
                                       cadmium::log::to_sim_double(to));
                }

                template<typename SINK>
                static void sim_local_time(const TIME&, const TIME&,
                                           const std::string& model_id) {
                    cadmium::log::emit(cadmium::log::level::debug, "simulator_local_time",
                                       "model=" + model_id);
                }

                template<typename SINK>
                static void run_global_time(const TIME& t) {
                    cadmium::log::emit(cadmium::log::level::debug, "simulation_tick", "",
                                       cadmium::log::to_sim_double(t));
                }

                template<typename SINK>
                static void run_info(const std::string& message) {
                    if (message == "Starting run") {
                        cadmium::log::emit(cadmium::log::level::info, "simulation_start", message);
                    } else if (message == "Finished run") {
                        cadmium::log::emit(cadmium::log::level::info, "simulation_end", message);
                    } else {
                        cadmium::log::emit(cadmium::log::level::info, "simulation_info", message);
                    }
                }
            };

        } // namespace logger
    } // namespace dynamic
} // namespace cadmium

#endif //CADMIUM_DYNAMIC_COMMON_LOGGERS_HPP
