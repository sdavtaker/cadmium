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

#ifndef CADMIUM_STDEVS_ENGINE_HELPERS_HPP
#define CADMIUM_STDEVS_ENGINE_HELPERS_HPP

#include <cadmium/concepts/stdevs_concepts.hpp>
#include <cadmium/engine/devs_engine_helpers.hpp>

#include <tuple>

namespace cadmium {
    namespace engine {
        namespace stdevs {

            // ── Forward declarations ──────────────────────────────────────────────

            template <template <typename T> class MODEL, typename TIME, typename URNG>
                requires cadmium::concepts::stdevs::AtomicModel<MODEL<TIME>, TIME, URNG>
            class simulator;

            template <template <typename T> class MODEL, typename TIME, typename URNG>
                requires cadmium::concepts::stdevs::CoupledModel<MODEL<TIME>>
            class coordinator;

            // ── Engine-type selector ──────────────────────────────────────────────
            //
            // Maps (IsAtomic, MODEL, TIME, URNG) → simulator or coordinator.

            template <bool IsAtomic, template <typename> class M, typename TIME, typename URNG>
            struct stdevs_select_engine_type;

            template <template <typename> class M, typename TIME, typename URNG>
            struct stdevs_select_engine_type<true, M, TIME, URNG> {
                using type = simulator<M, TIME, URNG>;
            };

            template <template <typename> class M, typename TIME, typename URNG>
            struct stdevs_select_engine_type<false, M, TIME, URNG> {
                using type = coordinator<M, TIME, URNG>;
            };

            // ── Coordinator tuple construction ────────────────────────────────────
            //
            // Maps each submodel template to its STDEVS engine type.

            template <typename TIME, typename URNG, template <typename> class MT, std::size_t S,
                      typename... COS>
            struct stdevs_coordinate_tuple_impl {
                template <typename T> using current = std::tuple_element_t<S - 1, MT<T>>;
                using current_coordinated           = typename stdevs_select_engine_type<
                              cadmium::concepts::stdevs::AtomicModel<current<float>, float, URNG>, current,
                              TIME, URNG>::type;
                using type =
                    typename stdevs_coordinate_tuple_impl<TIME, URNG, MT, S - 1,
                                                          current_coordinated, COS...>::type;
            };

            template <typename TIME, typename URNG, template <typename> class MT, typename... COS>
            struct stdevs_coordinate_tuple_impl<TIME, URNG, MT, 0, COS...> {
                using type = std::tuple<COS...>;
            };

            template <typename TIME, typename URNG, template <typename> class MT>
            struct stdevs_coordinate_tuple {
                using type =
                    typename stdevs_coordinate_tuple_impl<TIME, URNG, MT,
                                                          std::tuple_size<MT<float>>::value>::type;
            };

            // ── Subcoordinator initialisation ─────────────────────────────────────
            //
            // Unlike devs::init_subcoordinators, this variant passes URNG& to each
            // sub-engine's init so the RNG reference propagates all the way to leaves.

            template <typename TIME, typename URNG, typename CST>
            void init_subcoordinators_stdevs(const TIME &t, CST &cs, URNG &rng) {
                auto init_one = [&t, &rng](auto &eng) -> void { eng.init(t, rng); };
                std::apply([&](auto &...engs) { (init_one(engs), ...); }, cs);
            }

            // ── Shared utilities from devs namespace ──────────────────────────────
            //
            // These are re-imported here so stdevs headers can use them without
            // a full devs:: qualification everywhere.
            //
            // Safe to reuse because STDEVS simulators/coordinators expose the same
            // outbox_port / set_inbox_port / inbox_empty / advance_simulation(TIME)
            // interface as their DEVS counterparts.

            using cadmium::engine::devs::advance_receivers_only;
            using cadmium::engine::devs::advance_selected_and_receivers;
            using cadmium::engine::devs::all_box_empty;
            using cadmium::engine::devs::collect_messages_by_eoc_devs;
            using cadmium::engine::devs::collect_selected_output;
            using cadmium::engine::devs::first_imminent;
            using cadmium::engine::devs::get_engine_by_model;
            using cadmium::engine::devs::min_next_in_tuple;
            using cadmium::engine::devs::print_box_by_port;
            using cadmium::engine::devs::route_external_input_coupled_messages_devs;
            using cadmium::engine::devs::route_internal_coupled_messages_devs;

        } // namespace stdevs
    } // namespace engine
} // namespace cadmium

#endif // CADMIUM_STDEVS_ENGINE_HELPERS_HPP
