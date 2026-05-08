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

#ifndef CADMIUM_DEVS_ENGINE_HELPERS_HPP
#define CADMIUM_DEVS_ENGINE_HELPERS_HPP

#include <cadmium/concepts/devs_concepts.hpp>
#include <cadmium/logger/cadmium_log.hpp>
#include <cadmium/logger/common_loggers_helpers.hpp>
#include <cadmium/modeling/message_box.hpp>

#include <algorithm>
#include <limits>
#include <ostream>
#include <stdexcept>
#include <tuple>
#include <typeinfo>

namespace cadmium {
    namespace engine {
        namespace devs {

            // ── Forward declarations ──────────────────────────────────────────────

            template <template <typename T> class MODEL, typename TIME>
                requires cadmium::concepts::devs::AtomicModel<MODEL<TIME>, TIME>
            class simulator;

            template <template <typename T> class MODEL, typename TIME>
                requires cadmium::concepts::devs::CoupledModel<MODEL<TIME>>
            class coordinator;

            // ── Message-box utilities ─────────────────────────────────────────────

            template <class BOX> bool all_box_empty(BOX const &box) {
                auto check_empty = [](auto const &...b) -> bool {
                    return (!b.message.has_value() && ...);
                };
                return std::apply(check_empty, box);
            }

            template <std::size_t S, typename... T> struct print_box_by_port_impl {
                using current_box = std::tuple_element_t<S - 1, std::tuple<T...>>;
                static void run(std::ostream &os, const std::tuple<T...> &b) {
                    print_box_by_port_impl<S - 1, T...>::run(os, b);
                    os << ", " << typeid(typename current_box::port).name() << ": ";
                    const auto &opt = cadmium::get_message<typename current_box::port>(b);
                    if (opt.has_value()) {
                        os << "{";
                        cadmium::logger::print_value_or_name(os, opt.value());
                        os << "}";
                    } else {
                        os << "{}";
                    }
                }
            };

            template <typename... T> struct print_box_by_port_impl<1, T...> {
                using current_box = std::tuple_element_t<0, std::tuple<T...>>;
                static void run(std::ostream &os, const std::tuple<T...> &b) {
                    os << typeid(typename current_box::port).name() << ": ";
                    const auto &opt = cadmium::get_message<typename current_box::port>(b);
                    if (opt.has_value()) {
                        os << "{";
                        cadmium::logger::print_value_or_name(os, opt.value());
                        os << "}";
                    } else {
                        os << "{}";
                    }
                }
            };

            template <typename... T> struct print_box_by_port_impl<0, T...> {
                static void run(std::ostream &, const std::tuple<T...> &) {}
            };

            template <typename... T>
            void print_box_by_port(std::ostream &os, const std::tuple<T...> &b) {
                os << "[";
                print_box_by_port_impl<sizeof...(T), T...>::run(os, b);
                os << "]";
            }

            // ── Coordinator tuple construction ────────────────────────────────────
            //
            // Maps each submodel template to its engine type: simulator if atomic,
            // coordinator otherwise.

            template <bool IsAtomic, template <typename> class M, typename TIME>
            struct devs_select_engine_type;

            template <template <typename> class M, typename TIME>
            struct devs_select_engine_type<true, M, TIME> {
                using type = simulator<M, TIME>;
            };

            template <template <typename> class M, typename TIME>
            struct devs_select_engine_type<false, M, TIME> {
                using type = coordinator<M, TIME>;
            };

            template <typename TIME, template <typename> class MT, std::size_t S, typename... COS>
            struct devs_coordinate_tuple_impl {
                template <typename T> using current = std::tuple_element_t<S - 1, MT<T>>;
                using current_coordinated           = typename devs_select_engine_type<
                              cadmium::concepts::devs::AtomicModel<current<float>, float>, current,
                              TIME>::type;
                using type = typename devs_coordinate_tuple_impl<TIME, MT, S - 1,
                                                                 current_coordinated, COS...>::type;
            };

            template <typename TIME, template <typename> class MT, typename... COS>
            struct devs_coordinate_tuple_impl<TIME, MT, 0, COS...> {
                using type = std::tuple<COS...>;
            };

            template <typename TIME, template <typename> class MT> struct devs_coordinate_tuple {
                using type =
                    typename devs_coordinate_tuple_impl<TIME, MT,
                                                        std::tuple_size<MT<float>>::value>::type;
            };

            // ── Engine-by-model compile-time lookup ───────────────────────────────

            struct NO_ENGINE {};

            template <typename TIMED_MODEL, typename CST, std::size_t S>
            struct get_engine_by_model_impl {
                using current_engine = std::tuple_element_t<S - 1, CST>;
                using current_model  = typename current_engine::model_type;
                using type           = std::conditional_t<
                              std::is_same_v<current_model, TIMED_MODEL>, current_engine,
                              typename get_engine_by_model_impl<TIMED_MODEL, CST, S - 1>::type>;
            };

            template <typename TIMED_MODEL, typename CST>
            struct get_engine_by_model_impl<TIMED_MODEL, CST, 1> {
                using current_engine = std::tuple_element_t<0, CST>;
                using current_model  = typename current_engine::model_type;
                using type = std::conditional_t<std::is_same_v<current_model, TIMED_MODEL>,
                                                current_engine, NO_ENGINE>;
            };

            template <typename TIMED_MODEL, typename CST> struct get_engine_type_by_model {
                using type = typename get_engine_by_model_impl<TIMED_MODEL, CST,
                                                               std::tuple_size_v<CST>>::type;
            };

            template <typename TIMED_MODEL, typename CST>
            typename get_engine_type_by_model<TIMED_MODEL, CST>::type &
            get_engine_by_model(CST &cst) {
                return std::get<typename get_engine_type_by_model<TIMED_MODEL, CST>::type>(cst);
            }

            // ── Subcoordinator initialisation ─────────────────────────────────────

            template <typename TIME, typename CST>
            void init_subcoordinators(const TIME &t, CST &cs) {
                auto init_coordinator = [&t](auto &c) -> void { c.init(t); };
                std::apply([&](auto &...engs) { (init_coordinator(engs), ...); }, cs);
            }

            // ── min_next across the subcoordinator tuple ──────────────────────────

            template <typename T, std::size_t S> struct min_next_in_tuple_impl {
                static auto value(T &t) {
                    return std::min(std::get<S - 1>(t).next(),
                                    min_next_in_tuple_impl<T, S - 1>::value(t));
                }
            };

            template <typename T> struct min_next_in_tuple_impl<T, 1> {
                static auto value(T &t) {
                    return std::get<0>(t).next();
                }
            };

            template <typename T> auto min_next_in_tuple(T &t) {
                return min_next_in_tuple_impl<T, std::tuple_size<T>::value>::value(t);
            }

            // ── Default SELECT: picks the first imminent model by tuple index ──────

            struct first_imminent {
                template <typename CST, typename TIME>
                static std::size_t apply(const CST &cs, TIME t_n) {
                    std::size_t result = std::numeric_limits<std::size_t>::max();
                    std::size_t i      = 0;
                    auto f             = [&](const auto &eng) {
                        if (result == std::numeric_limits<std::size_t>::max() &&
                            eng.next() == t_n) {
                            result = i;
                        }
                        ++i;
                    };
                    std::apply([&](const auto &...engs) { (f(engs), ...); }, cs);
                    if (result == std::numeric_limits<std::size_t>::max()) {
                        throw std::domain_error("SELECT: no imminent model found at t_n");
                    }
                    return result;
                }
            };

            // ── Selective collect / advance ───────────────────────────────────────

            // Call collect_outputs(t) on the subengine at tuple index `selected`.
            template <typename TIME, typename CST>
            void collect_selected_output(const TIME &t, CST &cs, std::size_t selected) {
                std::size_t i = 0;
                auto f        = [&](auto &eng) {
                    if (i++ == selected)
                        eng.collect_outputs(t);
                };
                std::apply([&](auto &...engs) { (f(engs), ...); }, cs);
            }

            // Advance the selected engine plus every engine whose inbox is non-empty.
            // Other imminent models are left untouched (SELECT serialises them).
            template <typename TIME, typename CST>
            void advance_selected_and_receivers(const TIME &t, CST &cs, std::size_t selected) {
                std::size_t i = 0;
                auto f        = [&](auto &eng) {
                    if (i++ == selected || !eng.inbox_empty())
                        eng.advance_simulation(t);
                };
                std::apply([&](auto &...engs) { (f(engs), ...); }, cs);
            }

            // Advance only engines with non-empty inbox (EIC external-input case).
            template <typename TIME, typename CST>
            void advance_receivers_only(const TIME &t, CST &cs) {
                auto f = [&](auto &eng) {
                    if (!eng.inbox_empty())
                        eng.advance_simulation(t);
                };
                std::apply([&](auto &...engs) { (f(engs), ...); }, cs);
            }

            // ── EOC routing (message_box edition) ────────────────────────────────

            template <typename TIME, typename EOC, std::size_t S, typename OUT_BOX, typename CST>
            struct collect_messages_by_eoc_devs_impl {
                using current_eoc  = std::tuple_element_t<S - 1, EOC>;
                using ext_out_port = typename current_eoc::external_output_port;
                using sub_from     = typename current_eoc::template submodel<TIME>;
                using sub_out_port = typename current_eoc::submodel_output_port;

                static void fill(OUT_BOX &out, CST &cst) {
                    const auto &from_opt = get_engine_by_model<sub_from, CST>(cst)
                                               .template outbox_port<sub_out_port>();
                    if (from_opt.has_value()) {
                        cadmium::get_message<ext_out_port>(out) = from_opt;
                        std::ostringstream oss;
                        cadmium::logger::print_value_or_name(oss, from_opt.value());
                        cadmium::log::emit(cadmium::log::level::debug, "coor_routing_collect_eoc",
                                           std::string(typeid(sub_from).name()) + " [" +
                                               typeid(sub_out_port).name() + "] -> [" +
                                               typeid(ext_out_port).name() + "] " + oss.str());
                    }
                    collect_messages_by_eoc_devs_impl<TIME, EOC, S - 1, OUT_BOX, CST>::fill(out,
                                                                                            cst);
                }
            };

            template <typename TIME, typename EOC, typename OUT_BOX, typename CST>
            struct collect_messages_by_eoc_devs_impl<TIME, EOC, 0, OUT_BOX, CST> {
                static void fill(OUT_BOX &, CST &) {}
            };

            template <typename TIME, typename EOC, typename OUT_BOX, typename CST>
            OUT_BOX collect_messages_by_eoc_devs(CST &cst) {
                OUT_BOX ret{};
                collect_messages_by_eoc_devs_impl<TIME, EOC, std::tuple_size_v<EOC>, OUT_BOX,
                                                  CST>::fill(ret, cst);
                return ret;
            }

            // ── IC routing (message_box edition) ──────────────────────────────────

            template <typename TIME, typename CST, typename ICs, std::size_t S>
            struct route_ic_devs_impl {
                using current_IC = std::tuple_element_t<S - 1, ICs>;
                using from_model = typename current_IC::template from_model<TIME>;
                using from_port  = typename current_IC::from_model_output_port;
                using to_model   = typename current_IC::template to_model<TIME>;
                using to_port    = typename current_IC::to_model_input_port;

                static void route(const TIME &t, CST &engines) {
                    auto &from_engine = get_engine_by_model<from_model, CST>(engines);
                    auto &to_engine   = get_engine_by_model<to_model, CST>(engines);

                    const auto &from_opt = from_engine.template outbox_port<from_port>();
                    if (from_opt.has_value()) {
                        to_engine.template set_inbox_port<to_port>(from_opt);
                        std::ostringstream oss;
                        cadmium::logger::print_value_or_name(oss, from_opt.value());
                        cadmium::log::emit(cadmium::log::level::debug, "coor_routing_collect_ic",
                                           std::string(typeid(from_model).name()) + " [" +
                                               typeid(from_port).name() + "] -> " +
                                               typeid(to_model).name() + " [" +
                                               typeid(to_port).name() + "] " + oss.str(),
                                           cadmium::log::to_sim_double(t));
                    }

                    route_ic_devs_impl<TIME, CST, ICs, S - 1>::route(t, engines);
                }
            };

            template <typename TIME, typename CST, typename ICs>
            struct route_ic_devs_impl<TIME, CST, ICs, 0> {
                static void route(const TIME &, CST &) {}
            };

            template <typename TIME, typename CST, typename ICs>
            void route_internal_coupled_messages_devs(const TIME &t, CST &cst) {
                route_ic_devs_impl<TIME, CST, ICs, std::tuple_size_v<ICs>>::route(t, cst);
            }

            // ── EIC routing (message_box edition) ────────────────────────────────

            template <typename TIME, typename IN_BOX, typename CST, typename EICs, std::size_t S>
            struct route_eic_devs_impl {
                static void route(const TIME &t, const IN_BOX &inbox, CST &engines) {
                    if constexpr (S != 0) {
                        using current_EIC = std::tuple_element_t<S - 1, EICs>;
                        using from_port   = typename current_EIC::external_input_port;
                        using to_model    = typename current_EIC::template submodel<TIME>;
                        using to_port     = typename current_EIC::submodel_input_port;

                        auto &to_engine      = get_engine_by_model<to_model, CST>(engines);
                        const auto &from_opt = cadmium::get_message<from_port>(inbox);
                        if (from_opt.has_value()) {
                            to_engine.template set_inbox_port<to_port>(from_opt);
                            std::ostringstream oss;
                            cadmium::logger::print_value_or_name(oss, from_opt.value());
                            cadmium::log::emit(cadmium::log::level::debug,
                                               "coor_routing_collect_eic",
                                               std::string("[") + typeid(from_port).name() +
                                                   "] -> " + typeid(to_model).name() + " [" +
                                                   typeid(to_port).name() + "] " + oss.str(),
                                               cadmium::log::to_sim_double(t));
                        }

                        route_eic_devs_impl<TIME, IN_BOX, CST, EICs, S - 1>::route(t, inbox,
                                                                                   engines);
                    }
                }
            };

            template <typename TIME, typename IN_BOX, typename CST, typename EICs>
            void route_external_input_coupled_messages_devs(const TIME &t, const IN_BOX &inbox,
                                                            CST &cst) {
                route_eic_devs_impl<TIME, IN_BOX, CST, EICs, std::tuple_size_v<EICs>>::route(
                    t, inbox, cst);
            }

        } // namespace devs
    } // namespace engine
} // namespace cadmium

#endif // CADMIUM_DEVS_ENGINE_HELPERS_HPP
