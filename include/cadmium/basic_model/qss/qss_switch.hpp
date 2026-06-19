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

#pragma once

#include <cadmium/modeling/message_box.hpp>
#include <cadmium/modeling/ports.hpp>

#include <limits>
#include <ostream>

namespace cadmium::basic_models::qss {

    // Time-independent port definitions shared by all qss_switch<T> instantiations.
    struct qss_switch_defs {
        struct in0 : public cadmium::in_port<double> {};
        struct in1 : public cadmium::in_port<double> {};
        struct in_sw : public cadmium::in_port<double> {};
        struct out : public cadmium::out_port<double> {};
    };

    /**
     * QSS Level-Triggered Switch (2-to-1 mux).
     *
     * Routes in0 to out when sw == 0, in1 to out otherwise.
     * Fires immediately (sigma=0) on every input update to propagate
     * the selected signal downstream.
     *
     * API matches PowerDEVS atomics/hyb/switch.cpp for QSS1.
     *
     * @tparam TIME  Simulation time type.
     */
    template <typename TIME> class qss_switch : public qss_switch_defs {
      public:
        using in0   = qss_switch_defs::in0;
        using in1   = qss_switch_defs::in1;
        using in_sw = qss_switch_defs::in_sw;
        using out   = qss_switch_defs::out;

        using input_ports  = std::tuple<in0, in1, in_sw>;
        using output_ports = std::tuple<out>;

        struct state_type {
            double u0;
            double u1;
            double sw;
            TIME sigma;

            friend std::ostream &operator<<(std::ostream &os, const state_type &s) {
                return os << "u0=" << s.u0 << " u1=" << s.u1 << " sw=" << s.sw
                          << " sigma=" << s.sigma;
            }
        };

        state_type state{0.0, 0.0, 0.0, std::numeric_limits<TIME>::infinity()};

        void internal_transition() {
            state.sigma = std::numeric_limits<TIME>::infinity();
        }

        void external_transition(TIME /*e*/,
                                 typename cadmium::make_message_box<input_ports>::type mb) {
            auto &m0  = cadmium::get_message<in0>(mb);
            auto &m1  = cadmium::get_message<in1>(mb);
            auto &msw = cadmium::get_message<in_sw>(mb);
            if (m0.has_value())
                state.u0 = m0.value();
            if (m1.has_value())
                state.u1 = m1.value();
            if (msw.has_value())
                state.sw = msw.value();
            state.sigma = TIME{0};
        }

        typename cadmium::make_message_box<output_ports>::type output() const {
            typename cadmium::make_message_box<output_ports>::type box;
            cadmium::get_message<out>(box).emplace(state.sw != 0.0 ? state.u1 : state.u0);
            return box;
        }

        TIME time_advance() const {
            return state.sigma;
        }
    };

} // namespace cadmium::basic_models::qss
