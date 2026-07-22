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

#ifndef CADMIUM_NAMED_HPP
#define CADMIUM_NAMED_HPP

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <ostream>
#include <string_view>

namespace cadmium {

    /**
     * A compile-time string literal usable as a non-type template
     * parameter (a "structural type", permitted as an NTTP since C++20).
     * Stores a fixed-size, always null-terminated buffer, so a value can
     * be compared and used to parameterize other templates entirely at
     * compile time — a property `model_name` below needs, so it stays
     * usable both for logging (a runtime string_view) and, later, for
     * compile-time coupling/model-identity checks.
     */
    template <std::size_t N> struct fixed_string {
        char data[N]{};

        constexpr fixed_string(const char (&str)[N]) {
            std::copy_n(str, N, data);
        }

        [[nodiscard]] constexpr std::string_view view() const {
            return std::string_view(data, N - 1); // exclude the trailing '\0'
        }

        constexpr operator std::string_view() const {
            return view();
        }

        friend std::ostream &operator<<(std::ostream &os, const fixed_string &s) {
            return os << s.view();
        }

        friend constexpr bool operator==(const fixed_string &, const fixed_string &) = default;
    };

    // Deduction guide: fixed_string{"foo"} deduces N from the literal
    // (including the trailing '\0'), so callers never spell out N.
    template <std::size_t N> fixed_string(const char (&)[N]) -> fixed_string<N>;

    /**
     * A model is Named if it exposes a `static constexpr` member
     * `model_name` convertible to std::string_view — deliberately a
     * structural check, not "derives from `named<...>`" specifically:
     * inheriting from `named<Name>` (below) is the recommended, ergonomic
     * way to satisfy this for a wrapper type, but a model that already
     * has other reasons to declare `model_name` directly (rather than via
     * this particular base) still counts as Named.
     */
    template <typename T>
    concept Named = requires {
        { T::model_name } -> std::convertible_to<std::string_view>;
    };

    /**
     * Convenience CRTP-style base giving any type a compile-time name
     * with no other changes required: models_tuple/IC/EIC/EOC keep
     * accepting bare `template<typename> class` submodels exactly as
     * before, so this is entirely opt-in and non-breaking. Matches the
     * common pattern of wrapping a parameterized/non-default-constructible
     * model in a thin derived struct to satisfy models_tuple's
     * default-constructible requirement — that wrapper can simply also
     * inherit from `named<"...">` to pick up a name for free:
     *
     *   template <typename TIME>
     *   struct my_channel : bottleneck_channel<TIME, my_frame>, named<"my_channel"> {
     *       my_channel() : bottleneck_channel<TIME, my_frame>(0.05, 1000.0) {}
     *   };
     *
     * Models with nothing to configure (e.g. a bare `generator<TIME>` used
     * directly in a models_tuple) are not expected to opt in — they keep
     * the existing fallback name (typeid-based) unchanged.
     */
    template <fixed_string Name> struct named {
        static constexpr auto model_name = Name;
    };

} // namespace cadmium

#endif // CADMIUM_NAMED_HPP
