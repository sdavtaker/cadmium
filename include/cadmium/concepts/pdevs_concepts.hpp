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

#ifndef CADMIUM_PDEVS_CONCEPTS_HPP
#define CADMIUM_PDEVS_CONCEPTS_HPP

#include <cadmium/concepts/common_concepts.hpp>
#include <cadmium/modeling/message_bag.hpp>

namespace cadmium::concepts {

    // ─── PDEVS modeling-language concepts ────────────────────────────────────────

    namespace pdevs {

        // ── Coupling entry concepts ───────────────────────────────────────────────

        template <typename E>
        concept EICEntry = requires {
            typename E::external_input_port;
            typename E::submodel_input_port;
            requires InputPort<typename E::external_input_port>;
            requires InputPort<typename E::submodel_input_port>;
            requires std::same_as<typename E::external_input_port::message_type,
                                  typename E::submodel_input_port::message_type>;
            requires detail::port_in_tuple_v<typename E::submodel_input_port,
                                             typename E::template submodel<float>::input_ports>;
        };

        template <typename E>
        concept EOCEntry = requires {
            typename E::external_output_port;
            typename E::submodel_output_port;
            requires OutputPort<typename E::external_output_port>;
            requires OutputPort<typename E::submodel_output_port>;
            requires std::same_as<typename E::external_output_port::message_type,
                                  typename E::submodel_output_port::message_type>;
            requires detail::port_in_tuple_v<typename E::submodel_output_port,
                                             typename E::template submodel<float>::output_ports>;
        };

        template <typename E>
        concept ICEntry = requires {
            typename E::from_model_output_port;
            typename E::to_model_input_port;
            requires OutputPort<typename E::from_model_output_port>;
            requires InputPort<typename E::to_model_input_port>;
            requires std::same_as<typename E::from_model_output_port::message_type,
                                  typename E::to_model_input_port::message_type>;
            requires detail::port_in_tuple_v<typename E::from_model_output_port,
                                             typename E::template from_model<float>::output_ports>;
            requires detail::port_in_tuple_v<typename E::to_model_input_port,
                                             typename E::template to_model<float>::input_ports>;
            requires !std::same_as<typename E::template from_model<float>,
                                   typename E::template to_model<float>>;
        };

        template <typename Tuple> struct all_eic_entries_impl : std::false_type {};
        template <typename... Ts>
        struct all_eic_entries_impl<std::tuple<Ts...>> : std::bool_constant<(EICEntry<Ts> && ...)> {
        };
        template <typename Tuple>
        inline constexpr bool all_eic_entries_v = all_eic_entries_impl<Tuple>::value;

        template <typename Tuple> struct all_eoc_entries_impl : std::false_type {};
        template <typename... Ts>
        struct all_eoc_entries_impl<std::tuple<Ts...>> : std::bool_constant<(EOCEntry<Ts> && ...)> {
        };
        template <typename Tuple>
        inline constexpr bool all_eoc_entries_v = all_eoc_entries_impl<Tuple>::value;

        template <typename Tuple> struct all_ic_entries_impl : std::false_type {};
        template <typename... Ts>
        struct all_ic_entries_impl<std::tuple<Ts...>> : std::bool_constant<(ICEntry<Ts> && ...)> {};
        template <typename Tuple>
        inline constexpr bool all_ic_entries_v = all_ic_entries_impl<Tuple>::value;

        // ── AtomicModel ──────────────────────────────────────────────────────────

        /**
         * AtomicModel<M, TIME> — satisfied when M implements the five PDEVS functions
         * with the correct signatures for the given TIME type, declares state_type/
         * input_ports/output_ports, and has no duplicate port types.
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
            requires(M m, TIME e,
                     typename make_message_bags<typename M::input_ports>::type in_bags) {
                { m.state } -> std::same_as<typename M::state_type &>;
                { m.internal_transition() } -> std::same_as<void>;
                { m.external_transition(e, in_bags) } -> std::same_as<void>;
                { m.confluence_transition(e, in_bags) } -> std::same_as<void>;
                {
                    m.output()
                } -> std::same_as<typename make_message_bags<typename M::output_ports>::type>;
                { m.time_advance() } -> std::same_as<TIME>;
            };

        // ── CoupledModel ─────────────────────────────────────────────────────────

        /**
         * CoupledModel<M> — satisfied when M declares all structural members the PDEVS
         * coordinator expects, has unique port types, and every EIC/EOC/IC entry is
         * well-formed.
         */
        template <typename M>
        concept CoupledModel = requires {
            typename M::input_ports;
            typename M::output_ports;
            typename M::external_input_couplings;
            typename M::external_output_couplings;
            typename M::internal_couplings;
            typename M::template models<float>;
            requires detail::all_unique_types_v<typename M::input_ports>;
            requires detail::all_unique_types_v<typename M::output_ports>;
            requires all_eic_entries_v<typename M::external_input_couplings>;
            requires all_eoc_entries_v<typename M::external_output_couplings>;
            requires all_ic_entries_v<typename M::internal_couplings>;
        };

    } // namespace pdevs

} // namespace cadmium::concepts

#endif // CADMIUM_PDEVS_CONCEPTS_HPP
