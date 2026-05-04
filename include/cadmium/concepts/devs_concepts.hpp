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

#ifndef CADMIUM_DEVS_CONCEPTS_HPP
#define CADMIUM_DEVS_CONCEPTS_HPP

#include <cadmium/concepts/common_concepts.hpp>
#include <cadmium/modeling/message_box.hpp>

namespace cadmium::concepts {

    // ─── Classic DEVS atomic model concept ───────────────────────────────────────

    namespace devs {

        /**
         * AtomicModel<M, TIME> for the classic DEVS formalism.
         * Differences from PDEVS: external_transition takes a message_box (not bags),
         * and there is no confluence_transition.
         */
        template <typename M, typename TIME>
        concept AtomicModel =
            Time<TIME> &&
            requires {
                typename M::state_type;
                typename M::input_ports;
                typename M::output_ports;
                requires detail::all_unique_types_v<typename M::input_ports>;
                requires detail::all_unique_types_v<typename M::output_ports>;
            } &&
            requires(M m, TIME e, typename make_message_box<typename M::input_ports>::type in_box) {
                { m.state } -> std::same_as<typename M::state_type &>;
                { m.internal_transition() } -> std::same_as<void>;
                { m.external_transition(e, in_box) } -> std::same_as<void>;
                {
                    m.output()
                } -> std::same_as<typename make_message_box<typename M::output_ports>::type>;
                { m.time_advance() } -> std::same_as<TIME>;
            };

    } // namespace devs

} // namespace cadmium::concepts

#endif // CADMIUM_DEVS_CONCEPTS_HPP
