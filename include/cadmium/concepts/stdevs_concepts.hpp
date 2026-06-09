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

#ifndef CADMIUM_STDEVS_CONCEPTS_HPP
#define CADMIUM_STDEVS_CONCEPTS_HPP

#include <cadmium/concepts/common_concepts.hpp>
#include <cadmium/concepts/devs_concepts.hpp>
#include <cadmium/modeling/message_box.hpp>

namespace cadmium::concepts::stdevs {

    /**
     * AtomicModel<M, TIME, URNG> — the STDEVS atomic model concept.
     *
     * Based on classical DEVS (CKW10): internal and external transitions receive
     * a uniform random bit generator so that the user can sample from any
     * std::<random> distribution directly inside the transition body (Activity c,
     * CKW10 Fig. 3).  output() and time_advance() remain deterministic — λ and ta
     * must be measurable functions (CKW10 Section 4.2).
     *
     * Differences from devs::AtomicModel:
     *   internal_transition(rng)          — takes URNG& instead of no arguments
     *   external_transition(e, box, rng)  — takes URNG& as third argument
     *
     * CoupledModel structure is identical to devs::CoupledModel — see below.
     */
    template <typename M, typename TIME, typename URNG>
    concept AtomicModel =
        Time<TIME> &&
        requires {
            typename M::state_type;
            typename M::input_ports;
            typename M::output_ports;
            requires detail::all_unique_types_v<typename M::input_ports>;
            requires detail::all_unique_types_v<typename M::output_ports>;
        } &&
        requires(M m, TIME e, typename make_message_box<typename M::input_ports>::type in_box,
                 URNG &rng) {
            { m.state } -> std::same_as<typename M::state_type &>;
            { m.internal_transition(rng) } -> std::same_as<void>;
            { m.external_transition(e, in_box, rng) } -> std::same_as<void>;
            {
                m.output()
            } -> std::same_as<typename make_message_box<typename M::output_ports>::type>;
            { m.time_advance() } -> std::same_as<TIME>;
        };

    /**
     * CoupledModel<M> — the STDEVS coupled model concept.
     *
     * The coupling structure of STDEVS is identical to that of classic DEVS
     * (CKW10 Section 4.3): same port system, same EIC/EOC/IC entries, same Select
     * tie-breaking function.  This alias makes the equivalence explicit and allows
     * STDEVS coordinators to accept any devs::CoupledModel without a separate
     * hierarchy.
     */
    template <typename M>
    concept CoupledModel = cadmium::concepts::devs::CoupledModel<M>;

} // namespace cadmium::concepts::stdevs

#endif // CADMIUM_STDEVS_CONCEPTS_HPP
