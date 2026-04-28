/**
 * Copyright (c) 2013-present, Damian Vicino, Laouen M. L. Belloli
 * Carleton University, Universite de Nice-Sophia Antipolis, Universidad de Buenos Aires
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

#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <sstream>
#include <iostream>

namespace cadmium {
    namespace logger {

        struct logger_source{};
        struct logger_event{};

        // Event tags — coordinators
        struct coor_info_init             : public logger_event{};
        struct coor_info_collect          : public logger_event{};
        struct coor_routing_collect       : public logger_event{};
        struct coor_routing_collect_ic    : public logger_event{};
        struct coor_routing_collect_eic   : public logger_event{};
        struct coor_routing_collect_eoc   : public logger_event{};
        struct coor_info_advance          : public logger_event{};
        struct coor_routing_ic_collect    : public logger_event{};
        struct coor_routing_eic_collect   : public logger_event{};
        struct coor_routing_eoc_collect   : public logger_event{};

        // Event tags — simulators
        struct sim_info_init              : public logger_event{};
        struct sim_state                  : public logger_event{};
        struct sim_info_collect           : public logger_event{};
        struct sim_messages_collect       : public logger_event{};
        struct sim_info_advance           : public logger_event{};
        struct sim_local_time             : public logger_event{};

        // Event tags — runner
        struct run_global_time            : public logger_event{};
        struct run_info                   : public logger_event{};

        // Source (filter) tags
        struct logger_info                : public logger_source{};
        struct logger_debug               : public logger_source{};
        struct logger_state               : public logger_source{};
        struct logger_messages            : public logger_source{};
        struct logger_message_routing     : public logger_source{};
        struct logger_global_time         : public logger_source{};
        struct logger_local_time          : public logger_source{};

        // Each formatter method is now:
        //   template<typename SINK> static void event_name(args...) { ... }
        // The formatter owns the output mechanism — SINK-based or spdlog-based.

        template<typename LOGGER_SOURCE, class FORMATTER, typename SINK_PROVIDER>
        struct logger {
            template<typename DECLARED_SOURCE, typename EVENT, typename... PARAMs>
            static void log(const PARAMs&... ps) {
                if constexpr (std::is_same_v<LOGGER_SOURCE, DECLARED_SOURCE>) {
                    if constexpr (std::is_same_v<EVENT, coor_info_init>)
                        FORMATTER::template coor_info_init<SINK_PROVIDER>(ps...);
                    else if constexpr (std::is_same_v<EVENT, coor_info_collect>)
                        FORMATTER::template coor_info_collect<SINK_PROVIDER>(ps...);
                    else if constexpr (std::is_same_v<EVENT, coor_routing_collect>)
                        FORMATTER::template coor_routing_collect<SINK_PROVIDER>(ps...);
                    else if constexpr (std::is_same_v<EVENT, coor_routing_collect_ic>)
                        FORMATTER::template coor_routing_collect_ic<SINK_PROVIDER>(ps...);
                    else if constexpr (std::is_same_v<EVENT, coor_routing_collect_eic>)
                        FORMATTER::template coor_routing_collect_eic<SINK_PROVIDER>(ps...);
                    else if constexpr (std::is_same_v<EVENT, coor_routing_collect_eoc>)
                        FORMATTER::template coor_routing_collect_eoc<SINK_PROVIDER>(ps...);
                    else if constexpr (std::is_same_v<EVENT, coor_info_advance>)
                        FORMATTER::template coor_info_advance<SINK_PROVIDER>(ps...);
                    else if constexpr (std::is_same_v<EVENT, coor_routing_ic_collect>)
                        FORMATTER::template coor_routing_ic_collect<SINK_PROVIDER>(ps...);
                    else if constexpr (std::is_same_v<EVENT, coor_routing_eic_collect>)
                        FORMATTER::template coor_routing_eic_collect<SINK_PROVIDER>(ps...);
                    else if constexpr (std::is_same_v<EVENT, coor_routing_eoc_collect>)
                        FORMATTER::template coor_routing_eoc_collect<SINK_PROVIDER>(ps...);
                    else if constexpr (std::is_same_v<EVENT, sim_info_init>)
                        FORMATTER::template sim_info_init<SINK_PROVIDER>(ps...);
                    else if constexpr (std::is_same_v<EVENT, sim_state>)
                        FORMATTER::template sim_state<SINK_PROVIDER>(ps...);
                    else if constexpr (std::is_same_v<EVENT, sim_info_collect>)
                        FORMATTER::template sim_info_collect<SINK_PROVIDER>(ps...);
                    else if constexpr (std::is_same_v<EVENT, sim_messages_collect>)
                        FORMATTER::template sim_messages_collect<SINK_PROVIDER>(ps...);
                    else if constexpr (std::is_same_v<EVENT, sim_info_advance>)
                        FORMATTER::template sim_info_advance<SINK_PROVIDER>(ps...);
                    else if constexpr (std::is_same_v<EVENT, sim_local_time>)
                        FORMATTER::template sim_local_time<SINK_PROVIDER>(ps...);
                    else if constexpr (std::is_same_v<EVENT, run_global_time>)
                        FORMATTER::template run_global_time<SINK_PROVIDER>(ps...);
                    else if constexpr (std::is_same_v<EVENT, run_info>)
                        FORMATTER::template run_info<SINK_PROVIDER>(ps...);
                }
            }
        };

        template<typename... LS>
        struct multilogger_impl;

        template<typename L, typename... LS>
        struct multilogger_impl<L, LS...> {
            template<typename DECLARED_SOURCE, typename EVENT, typename... PARAMs>
            static void log(const PARAMs&... ps) {
                L::template log<DECLARED_SOURCE, EVENT, PARAMs...>(ps...);
                multilogger_impl<LS...>::template log<DECLARED_SOURCE, EVENT, PARAMs...>(ps...);
            }
        };

        template<>
        struct multilogger_impl<> {
            template<typename DECLARED_SOURCE, typename EVENT, typename... PARAMs>
            static void log(const PARAMs&...) {}
        };

        template<typename... LS>
        struct multilogger {
            template<typename DECLARED_SOURCE, typename EVENT, typename... PARAMs>
            static void log(const PARAMs&... ps) {
                multilogger_impl<LS...>::template log<DECLARED_SOURCE, EVENT, PARAMs...>(ps...);
            }
        };

    } // namespace logger
} // namespace cadmium

#endif // LOGGER_HPP
