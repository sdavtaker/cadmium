/**
 * Copyright (c) 2017-present, Laouen M. L. Belloli
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

#pragma once

#include <algorithm>
#include <map>
#include <memory>
#include <vector>

#include <cadmium/modeling/dynamic_message_bag.hpp>
#include <cadmium/engine/pdevs_dynamic_engine.hpp>
#include <cadmium/logger/common_loggers.hpp>

namespace cadmium {
    namespace dynamic {
        namespace engine {

            template<class BOX>
            decltype(auto) all_bags_empty(cadmium::dynamic::message_bags const& dynamic_bag) {
                auto is_empty = [&dynamic_bag](auto b) -> bool {
                    using bag_type = decltype(b);
                    using port_type = typename bag_type::port;
                    if (dynamic_bag.find(typeid(port_type)) != dynamic_bag.cend()) {
                        return boost::any_cast<bag_type>(dynamic_bag.at(typeid(port_type))).messages.empty();
                    }
                    return true;
                };
                auto check_empty = [&is_empty](auto const&... b) -> decltype(auto) {
                    return (is_empty(b) && ...);
                };
                BOX box;
                return std::apply(check_empty, box);
            }

            template<typename TIME>
            using subcoordinators_type =
                std::vector<std::shared_ptr<cadmium::dynamic::engine::engine<TIME>>>;

            template<typename TIME>
            using external_coupling = std::pair<
                std::shared_ptr<cadmium::dynamic::engine::engine<TIME>>,
                std::vector<std::shared_ptr<cadmium::dynamic::engine::link_abstract>>
            >;

            template<typename TIME>
            using external_couplings = std::vector<external_coupling<TIME>>;

            template<typename TIME>
            using internal_coupling = std::pair<
                std::pair<
                    std::shared_ptr<cadmium::dynamic::engine::engine<TIME>>,
                    std::shared_ptr<cadmium::dynamic::engine::engine<TIME>>
                >,
                std::vector<std::shared_ptr<cadmium::dynamic::engine::link_abstract>>
            >;

            template<typename TIME>
            using internal_couplings = std::vector<internal_coupling<TIME>>;

            template<typename TIME>
            void init_subcoordinators(TIME t, subcoordinators_type<TIME>& subcoordinators) {
                for (auto& c : subcoordinators) {
                    c->init(t);
                }
            }

            template<typename TIME>
            void advance_simulation_in_subengines(TIME t, subcoordinators_type<TIME>& subcoordinators) {
                for (auto& c : subcoordinators) {
                    c->advance_simulation(t);
                }
            }

            template<typename TIME>
            void collect_outputs_in_subcoordinators(TIME t, subcoordinators_type<TIME>& subcoordinators) {
                for (auto& c : subcoordinators) {
                    c->collect_outputs(t);
                }
            }

            template<typename TIME, typename LOGGER>
            cadmium::dynamic::message_bags collect_messages_by_eoc(const external_couplings<TIME>& coupling) {
                cadmium::dynamic::message_bags ret;
                for (auto& c : coupling) {
                    cadmium::dynamic::message_bags outbox = c.first->outbox();
                    for (const auto& l : c.second) {
                        cadmium::dynamic::logger::routed_messages message_to_log =
                            l->route_messages(outbox, ret);
                        LOGGER::template log<cadmium::logger::logger_message_routing,
                                             cadmium::logger::coor_routing_collect>(
                            message_to_log.from_port, message_to_log.to_port,
                            message_to_log.from_messages, message_to_log.to_messages);
                    }
                }
                return ret;
            }

            template<typename TIME, typename LOGGER>
            void route_external_input_coupled_messages_on_subcoordinators(
                cadmium::dynamic::message_bags inbox,
                const external_couplings<TIME>& coupling)
            {
                for (auto& c : coupling) {
                    for (const auto& l : c.second) {
                        auto& to_inbox = c.first->inbox();
                        cadmium::dynamic::logger::routed_messages message_to_log =
                            l->route_messages(inbox, to_inbox);
                        LOGGER::template log<cadmium::logger::logger_message_routing,
                                             cadmium::logger::coor_routing_collect>(
                            message_to_log.from_port, message_to_log.to_port,
                            message_to_log.from_messages, message_to_log.to_messages);
                    }
                }
            }

            template<typename TIME, typename LOGGER>
            void route_internal_coupled_messages_on_subcoordinators(const internal_couplings<TIME>& coupling) {
                for (auto& c : coupling) {
                    for (const auto& l : c.second) {
                        auto& from_outbox = c.first.first->outbox();
                        auto& to_inbox = c.first.second->inbox();
                        cadmium::dynamic::logger::routed_messages message_to_log =
                            l->route_messages(from_outbox, to_inbox);
                        LOGGER::template log<cadmium::logger::logger_message_routing,
                                             cadmium::logger::coor_routing_collect>(
                            message_to_log.from_port, message_to_log.to_port,
                            message_to_log.from_messages, message_to_log.to_messages);
                    }
                }
            }

            template<typename TIME>
            TIME min_next_in_subcoordinators(const subcoordinators_type<TIME>& subcoordinators) {
                std::vector<TIME> next_times(subcoordinators.size());
                std::transform(
                    subcoordinators.cbegin(), subcoordinators.cend(),
                    next_times.begin(),
                    [](const auto& c) -> TIME { return c->next(); });
                return *std::min_element(next_times.begin(), next_times.end());
            }
        }
    }
}
