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

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace cadmium::basic_models::qss {

    /**
     * QSS1 Quantized Integrator (Kofman & Junco 2001, Section 4).
     *
     * Integrates dx/dt = u(t) using a piecewise-constant quantized approximation.
     * State variable x fires an output event whenever x crosses a quantization
     * boundary q ± dq, where dq = max(dqmin, dqrel * |x|).
     *
     * Ports:
     *   in_u  – derivative input u = dx/dt  (double)
     *   out_q – quantized state output q     (double)
     *
     * Signals in/out carry the value at the crossing boundary.  At t=0 the
     * integrator fires once with sigma=0 to announce its initial value.
     *
     * API matches PowerDEVS atomics/qss/qss.cpp for cross-validation.
     */
    template <typename TIME> class qss1_integrator {
      public:
        struct in_u : public cadmium::in_port<double> {};
        struct out_q : public cadmium::out_port<double> {};

        using input_ports  = std::tuple<in_u>;
        using output_ports = std::tuple<out_q>;

        struct state_type {
            double x;   // continuous state (piecewise-linear trajectory)
            double u;   // current derivative
            double q;   // last quantized output (hysteresis centre)
            double dq;  // current quantum size
            TIME sigma; // time to next internal transition
        };

        state_type state;

        const double dqmin;
        const double dqrel;

        qss1_integrator(double x0, double dqmin_, double dqrel_) : dqmin(dqmin_), dqrel(dqrel_) {
            state.u  = 0.0;
            state.x  = x0;
            state.q  = x0;
            state.dq = std::max(dqmin, std::abs(x0) * dqrel);
            if (state.dq == 0.0)
                state.dq = dqmin;
            state.sigma = TIME{0}; // announce initial value immediately
        }

        void internal_transition() {
            // Advance x exactly to the crossing boundary, update q, recompute sigma.
            state.x += state.u * static_cast<double>(state.sigma);
            state.q  = state.x;
            state.dq = std::max(dqmin, dqrel * std::abs(state.x));
            if (state.dq == 0.0)
                state.dq = dqmin;
            state.sigma = (state.u == 0.0) ? std::numeric_limits<TIME>::infinity()
                                           : static_cast<TIME>(state.dq / std::abs(state.u));
        }

        void external_transition(TIME e, typename cadmium::make_message_box<input_ports>::type mb) {
            auto &msg = cadmium::get_message<in_u>(mb);
            if (!msg.has_value())
                throw std::logic_error("qss1_integrator: external_transition with no message");

            state.x += state.u * static_cast<double>(e);
            state.u  = msg.value();
            state.dq = std::max(dqmin, dqrel * std::abs(state.x));
            if (state.dq == 0.0)
                state.dq = dqmin;

            if (state.u == 0.0) {
                state.sigma = std::numeric_limits<TIME>::infinity();
                return;
            }

            // Minimum positive time to reach q+dq or q-dq from current x.
            double t    = std::numeric_limits<double>::infinity();
            double t_up = (state.q + state.dq - state.x) / state.u;
            double t_dn = (state.q - state.dq - state.x) / state.u;
            if (t_up > 0.0)
                t = std::min(t, t_up);
            if (t_dn > 0.0)
                t = std::min(t, t_dn);
            // Already past a boundary
            if (std::abs(state.x - state.q) >= state.dq)
                t = 0.0;

            state.sigma = static_cast<TIME>(t);
        }

        typename cadmium::make_message_box<output_ports>::type output() const {
            typename cadmium::make_message_box<output_ports>::type box;
            // Output the value at the next crossing point: x + u*sigma.
            double val = state.x + state.u * static_cast<double>(state.sigma);
            cadmium::get_message<out_q>(box).emplace(val);
            return box;
        }

        TIME time_advance() const {
            return state.sigma;
        }
    };

} // namespace cadmium::basic_models::qss
