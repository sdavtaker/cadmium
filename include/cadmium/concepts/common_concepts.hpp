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

#ifndef CADMIUM_COMMON_CONCEPTS_HPP
#define CADMIUM_COMMON_CONCEPTS_HPP

#include <cadmium/modeling/ports.hpp>

#include <concepts>
#include <limits>
#include <tuple>
#include <type_traits>

namespace cadmium::concepts {

    namespace detail {
        template <typename Needle, typename... Ts>
        inline constexpr std::size_t count_in_pack = (std::is_same_v<Needle, Ts> + ... + 0);

        template <typename Tuple> struct all_unique_types_impl : std::false_type {};

        template <typename... Ts>
        struct all_unique_types_impl<std::tuple<Ts...>>
            : std::bool_constant<((count_in_pack<Ts, Ts...> == 1) && ...)> {};

        // True iff every type in the tuple is distinct.
        template <typename Tuple>
        inline constexpr bool all_unique_types_v = all_unique_types_impl<Tuple>::value;

        template <typename PORT, typename TUPLE> struct port_in_tuple_impl : std::false_type {};

        template <typename PORT, typename... Ps>
        struct port_in_tuple_impl<PORT, std::tuple<Ps...>>
            : std::bool_constant<(std::is_same_v<PORT, Ps> || ...)> {};

        template <typename PORT, typename TUPLE>
        inline constexpr bool port_in_tuple_v = port_in_tuple_impl<PORT, TUPLE>::value;

    } // namespace detail

    // ─── Port concepts ───────────────────────────────────────────────────────────

    template <typename P>
    concept Port = requires {
        typename P::message_type;
        requires std::same_as<const port_kind, decltype(P::kind)>;
    };

    template <typename P>
    concept InputPort = Port<P> && (P::kind == port_kind::in);

    template <typename P>
    concept OutputPort = Port<P> && (P::kind == port_kind::out);

    // ─── Time concept ────────────────────────────────────────────────────────────

    template <typename T>
    concept Time = std::totally_ordered<T> && std::is_arithmetic_v<T> && requires {
        { std::numeric_limits<T>::infinity() } -> std::convertible_to<T>;
    };

} // namespace cadmium::concepts

#endif // CADMIUM_COMMON_CONCEPTS_HPP
