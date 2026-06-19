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

    // Time-independent port definitions shared by all qss_cross_detect<T> instantiations.
    struct qss_cross_detect_defs {
        struct in_u : public cadmium::in_port<double> {};
        struct in_Du : public cadmium::in_port<double> {};
        struct out : public cadmium::out_port<double> {};
    };

    /**
     * QSS Zero-Crossing Detector.
     *
     * Predicts when signal u (received as quantized piecewise-constant updates)
     * with derivative Du will cross a fixed threshold.  On crossing, emits
     * +1.0 (upward) or -1.0 (downward).
     *
     * sigma = (threshold - u) / Du ; clamped to [0, inf].
     *
     * API matches PowerDEVS atomics/hyb/cross_detect.cpp for QSS1.
     *
     * @tparam TIME  Simulation time type.
     */
    template <typename TIME> class qss_cross_detect : public qss_cross_detect_defs {
      public:
        using in_u  = qss_cross_detect_defs::in_u;
        using in_Du = qss_cross_detect_defs::in_Du;
        using out   = qss_cross_detect_defs::out;

        using input_ports  = std::tuple<in_u, in_Du>;
        using output_ports = std::tuple<out>;

        struct state_type {
            double u;
            double Du;
            TIME sigma;

            friend std::ostream &operator<<(std::ostream &os, const state_type &s) {
                return os << "u=" << s.u << " Du=" << s.Du << " sigma=" << s.sigma;
            }
        };

        state_type state;
        const double threshold;

        explicit qss_cross_detect(double threshold_ = 0.0) : threshold(threshold_) {
            state.u     = 0.0;
            state.Du    = 0.0;
            state.sigma = std::numeric_limits<TIME>::infinity();
        }

        void internal_transition() {
            state.sigma = std::numeric_limits<TIME>::infinity();
        }

        void external_transition(TIME /*e*/,
                                 typename cadmium::make_message_box<input_ports>::type mb) {
            auto &mu  = cadmium::get_message<in_u>(mb);
            auto &mDu = cadmium::get_message<in_Du>(mb);
            if (mu.has_value())
                state.u = mu.value();
            if (mDu.has_value())
                state.Du = mDu.value();

            if (state.Du == 0.0) {
                state.sigma = std::numeric_limits<TIME>::infinity();
                return;
            }
            double t    = (threshold - state.u) / state.Du;
            state.sigma = (t <= 0.0) ? TIME{0} : static_cast<TIME>(t);
        }

        typename cadmium::make_message_box<output_ports>::type output() const {
            typename cadmium::make_message_box<output_ports>::type box;
            cadmium::get_message<out>(box).emplace(state.Du > 0.0 ? 1.0 : -1.0);
            return box;
        }

        TIME time_advance() const {
            return state.sigma;
        }
    };

} // namespace cadmium::basic_models::qss
