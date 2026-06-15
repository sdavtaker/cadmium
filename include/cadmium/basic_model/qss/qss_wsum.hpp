// SPDX-License-Identifier: BSD-2-Clause
/**
 * Copyright (c) 2026-present, Damian Vicino
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

#pragma once

#include <cadmium/modeling/message_box.hpp>
#include <cadmium/modeling/ports.hpp>

#include <array>
#include <limits>
#include <utility>

namespace cadmium::basic_models::qss {

    /**
     * QSS Weighted Sum (N inputs).
     *
     * Computes y = K[0]*u0 + K[1]*u1 + ... + K[N-1]*u(N-1).
     * Fires immediately (sigma=0) whenever any input is received.
     * Holds its last computed value between events.
     *
     * API matches PowerDEVS atomics/qss/qss_wsum.cpp for QSS1.
     *
     * @tparam N     Number of input ports (compile-time constant).
     * @tparam TIME  Simulation time type.
     */
    template <std::size_t N, typename TIME> class qss_wsum {
      public:
        // Input port tag parameterised by index — each is a distinct type.
        template <std::size_t I> struct in : public cadmium::in_port<double> {};
        struct out : public cadmium::out_port<double> {};

      private:
        template <std::size_t... Is> static auto make_input_ports_impl(std::index_sequence<Is...>) {
            return std::tuple<in<Is>...>{};
        }

      public:
        using input_ports  = decltype(make_input_ports_impl(std::make_index_sequence<N>{}));
        using output_ports = std::tuple<out>;

        struct state_type {
            std::array<double, N> inputs{};
            bool pending; // true when an input arrived and output not yet emitted
            TIME sigma;
        };

        state_type state;
        std::array<double, N> coeffs;

        explicit qss_wsum(std::array<double, N> k) : coeffs(k) {
            state.inputs.fill(0.0);
            state.pending = false;
            state.sigma   = std::numeric_limits<TIME>::infinity();
        }

        void internal_transition() {
            state.pending = false;
            state.sigma   = std::numeric_limits<TIME>::infinity();
        }

        void external_transition(TIME /*e*/,
                                 typename cadmium::make_message_box<input_ports>::type mb) {
            // Update whichever inputs arrived.
            [&]<std::size_t... Is>(std::index_sequence<Is...>) {
                (([]<std::size_t I>(auto &s, auto &box) {
                     auto &msg = cadmium::get_message<in<I>>(box);
                     if (msg.has_value())
                         s.inputs[I] = msg.value();
                 }.template operator()<Is>(state, mb)),
                 ...);
            }(std::make_index_sequence<N>{});

            state.pending = true;
            state.sigma   = TIME{0};
        }

        typename cadmium::make_message_box<output_ports>::type output() const {
            typename cadmium::make_message_box<output_ports>::type box;
            double y = 0.0;
            for (std::size_t i = 0; i < N; ++i)
                y += coeffs[i] * state.inputs[i];
            cadmium::get_message<out>(box).emplace(y);
            return box;
        }

        TIME time_advance() const {
            return state.sigma;
        }
    };

} // namespace cadmium::basic_models::qss
