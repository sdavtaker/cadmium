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

#ifndef CADMIUM_HELPERS_HPP
#define CADMIUM_HELPERS_HPP

#include <tuple>
#include <type_traits>

namespace cadmium {
    namespace model_checks {

        namespace { // details — internal linkage, accessible within each TU that
                    // includes this header

            template <typename, template <typename...> class>
            struct is_specialization : std::false_type {};
            template <template <typename...> class TEMP, typename... ARGS>
            struct is_specialization<TEMP<ARGS...>, TEMP> : std::true_type {};

            template <typename T> constexpr bool is_tuple() {
                return is_specialization<T, std::tuple>::value;
            }

            template <typename T, int S> struct check_unique_elem_types_impl {
                static consteval bool value() {
                    using elem = std::tuple_element_t<S - 1, T>;
                    (void)std::get<elem>(T{});
                    return check_unique_elem_types_impl<T, S - 1>::value();
                }
            };

            template <typename T> struct check_unique_elem_types_impl<T, 0> {
                static consteval bool value() {
                    return true;
                }
            };

            template <typename T> struct check_unique_elem_types {
                static consteval bool value() {
                    return check_unique_elem_types_impl<T, std::tuple_size_v<T>>::value();
                }
            };

            template <typename PORT, typename TUPLE, int S> struct has_port_in_tuple_impl {
                static consteval bool value() {
                    if (std::is_same_v<PORT, std::tuple_element_t<S - 1, TUPLE>>)
                        return true;
                    return has_port_in_tuple_impl<PORT, TUPLE, S - 1>::value();
                }
            };

            template <typename PORT, typename TUPLE> struct has_port_in_tuple_impl<PORT, TUPLE, 0> {
                static consteval bool value() {
                    return false;
                }
            };

            template <typename PORT, typename TUPLE> struct has_port_in_tuple {
                static consteval bool value() {
                    return has_port_in_tuple_impl<PORT, TUPLE, std::tuple_size_v<TUPLE>>::value();
                }
            };

        } // anonymous namespace
    } // namespace model_checks
} // namespace cadmium

#endif // CADMIUM_HELPERS_HPP
