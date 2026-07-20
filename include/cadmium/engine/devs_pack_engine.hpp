// SPDX-License-Identifier: BSD-2-Clause
/**
 * Copyright (c) 2026-present, Damian Vicino
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

#ifndef CADMIUM_DEVS_PACK_ENGINE_HPP
#define CADMIUM_DEVS_PACK_ENGINE_HPP

#include <cadmium/concepts/devs_concepts.hpp>
#include <cadmium/engine/devs_engine_helpers.hpp>

#include <cstddef>
#include <limits>
#include <optional>
#include <string>
#include <vector>

namespace cadmium {
    namespace engine {
        namespace devs {

            /**
             * Engine for a pack-of-models submodel (modeling::pack<M, N>):
             * holds N element engines (simulator for atomic elements,
             * coordinator for coupled ones) in a runtime vector and exposes
             * the per-element interface the coupled-model helpers address
             * through flat SELECT indices and (pack, index) couplings.
             *
             * Element model ids get an "[i]" suffix so per-element identity is
             * preserved in the NDJSON log.
             */
            template <template <typename T> class PACKM, typename TIME> class pack_engine {
                static_assert(cadmium::concepts::devs::PackModel<PACKM<TIME>>,
                              "pack_engine requires a pack marker model");

                using marker                            = PACKM<TIME>;
                template <typename T> using element_tpl = typename PACKM<T>::element_model;

                static constexpr bool element_is_atomic =
                    cadmium::concepts::devs::AtomicModel<element_tpl<float>, float>;
                using element_engine =
                    typename devs_select_engine_type<element_is_atomic, element_tpl, TIME>::type;

                std::vector<element_engine> _elements{};
                TIME _next{};

                void refresh_next() {
                    // pack_size > 0 is enforced by the marker's static_assert,
                    // so seeding from element 0 avoids requiring an infinity
                    // representation of TIME.
                    _next = _elements[0].next();
                    for (std::size_t i = 1; i < _elements.size(); ++i) {
                        if (_elements[i].next() < _next) {
                            _next = _elements[i].next();
                        }
                    }
                }

              public:
                using model_type = marker;

                static constexpr std::size_t pack_size = marker::pack_size;

                std::size_t element_count() const noexcept {
                    return pack_size;
                }

                void init(TIME t) {
                    _elements.clear();
                    _elements.resize(pack_size);
                    for (std::size_t i = 0; i < pack_size; ++i) {
                        _elements[i].set_model_id_suffix("[" + std::to_string(i) + "]");
                        _elements[i].init(t);
                    }
                    refresh_next();
                }

                TIME next() const noexcept {
                    return _next;
                }

                TIME next_of(std::size_t k) const {
                    return _elements[k].next();
                }

                void collect_outputs_of(std::size_t k, const TIME &t) {
                    _elements[k].collect_outputs(t);
                }

                bool inbox_empty() const noexcept {
                    for (const auto &e : _elements) {
                        if (!e.inbox_empty()) {
                            return false;
                        }
                    }
                    return true;
                }

                template <typename PORT>
                const std::optional<typename PORT::message_type> &
                element_outbox_port(std::size_t k) const noexcept {
                    return _elements[k].template outbox_port<PORT>();
                }

                template <typename PORT>
                void set_element_inbox_port(std::size_t k,
                                            std::optional<typename PORT::message_type> msg) {
                    _elements[k].template set_inbox_port<PORT>(std::move(msg));
                }

                /**
                 * Advance element `selected` (pass npos for none) plus every
                 * element with a non-empty inbox — the pack-local equivalent
                 * of advance_selected_and_receivers.
                 */
                static constexpr std::size_t npos = static_cast<std::size_t>(-1);

                void advance_element_and_receivers(const TIME &t, std::size_t selected) {
                    for (std::size_t i = 0; i < pack_size; ++i) {
                        if (i == selected || !_elements[i].inbox_empty()) {
                            _elements[i].advance_simulation(t);
                        }
                    }
                    refresh_next();
                }
            };

        } // namespace devs
    } // namespace engine
} // namespace cadmium

#endif // CADMIUM_DEVS_PACK_ENGINE_HPP
