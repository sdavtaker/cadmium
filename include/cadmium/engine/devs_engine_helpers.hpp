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

#include <cadmium/logger/common_loggers_helpers.hpp>
#include <cadmium/modeling/message_box.hpp>

#include <ostream>
#include <tuple>
#include <typeinfo>

namespace cadmium {
    namespace engine {
        namespace devs {

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

        } // namespace devs
    } // namespace engine
} // namespace cadmium

#endif // CADMIUM_DEVS_ENGINE_HELPERS_HPP
