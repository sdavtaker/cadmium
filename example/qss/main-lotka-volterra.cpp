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

/**
 * Lotka-Volterra predator-prey QSS1 experiment.
 *
 * Equivalent to PowerDEVS examples/continuous/lotka_volterra.
 *
 * ODEs:  dx1/dt =  0.1*x1 - 0.1*x1*x2   (prey)
 *        dx2/dt =  0.1*x1*x2 - 0.1*x2   (predator)
 * ICs:   x1(0) = x2(0) = 0.5
 * t:     [0, 300]
 * dqmin: 1e-6,  dqrel: 1e-3
 *
 * Output: CSV to stdout — t,x1,x2
 * One row per output event from either integrator.
 *
 * Coupling (manual PDEVS simulation loop):
 *   int0.out_q --> wsum0.in<0>  (x1 with coeff +0.1)
 *   int0.out_q --> mult.in0
 *   int1.out_q --> wsum1.in<1>  (x2 with coeff -0.1)
 *   int1.out_q --> mult.in1
 *   mult.out   --> wsum0.in<1>  (x1*x2 with coeff -0.1)
 *   mult.out   --> wsum1.in<0>  (x1*x2 with coeff +0.1)
 *   wsum0.out  --> int0.in_u
 *   wsum1.out  --> int1.in_u
 */

#include <cadmium/basic_model/qss/qss1_integrator.hpp>
#include <cadmium/basic_model/qss/qss_multiplier.hpp>
#include <cadmium/basic_model/qss/qss_wsum.hpp>

#include <array>
#include <iostream>
#include <limits>
#include <optional>

using namespace cadmium::basic_models::qss;
using cadmium::get_message;
using cadmium::make_message_box;

using TIME = double;

// ── Component aliases ────────────────────────────────────────────────────────

using Integrator = qss1_integrator<TIME>;
using WSum2      = qss_wsum<2, TIME>;
using Mult       = qss_multiplier<TIME>;

// ── Helpers: extract scalar from output box ──────────────────────────────────

static double get_int(const make_message_box<Integrator::output_ports>::type &b) {
    return get_message<Integrator::out_q>(b).value();
}
static double get_ws(const make_message_box<WSum2::output_ports>::type &b) {
    return get_message<WSum2::out>(b).value();
}
static double get_mult(const make_message_box<Mult::output_ports>::type &b) {
    return get_message<Mult::out>(b).value();
}

// ── Helpers: build input boxes ───────────────────────────────────────────────

static make_message_box<Integrator::input_ports>::type int_box(double u) {
    make_message_box<Integrator::input_ports>::type mb{};
    get_message<Integrator::in_u>(mb).emplace(u);
    return mb;
}

static make_message_box<WSum2::input_ports>::type ws_box(std::optional<double> v0,
                                                         std::optional<double> v1) {
    make_message_box<WSum2::input_ports>::type mb{};
    if (v0)
        get_message<WSum2::in<0>>(mb).emplace(*v0);
    if (v1)
        get_message<WSum2::in<1>>(mb).emplace(*v1);
    return mb;
}

static make_message_box<Mult::input_ports>::type mult_box(std::optional<double> v0,
                                                          std::optional<double> v1) {
    make_message_box<Mult::input_ports>::type mb{};
    if (v0)
        get_message<Mult::in0>(mb).emplace(*v0);
    if (v1)
        get_message<Mult::in1>(mb).emplace(*v1);
    return mb;
}

// ── Main ─────────────────────────────────────────────────────────────────────

int main() {
    constexpr TIME T_END   = 300.0;
    constexpr double DQMIN = 1e-6;
    constexpr double DQREL = 1e-3;
    constexpr TIME INF     = std::numeric_limits<TIME>::infinity();

    // Instantiate components
    Integrator int0(0.5, DQMIN, DQREL); // prey    x1(0)=0.5
    Integrator int1(0.5, DQMIN, DQREL); // predator x2(0)=0.5
    Mult mult{};
    WSum2 ws0({0.1, -0.1}); // dx1/dt = +0.1*x1 - 0.1*(x1*x2)
    WSum2 ws1({0.1, -0.1}); // dx2/dt = +0.1*(x1*x2) - 0.1*x2

    // Scheduled next-event times (absolute)
    TIME t_int0 = int0.time_advance(); // 0
    TIME t_int1 = int1.time_advance(); // 0
    TIME t_mult = INF;
    TIME t_ws0  = INF;
    TIME t_ws1  = INF;

    // Last time each component received an external event (for elapsed-time calc)
    TIME last_int0 = 0.0, last_int1 = 0.0;
    TIME last_mult = 0.0, last_ws0 = 0.0, last_ws1 = 0.0;

    std::cout << "t,x1,x2\n";

    while (true) {
        TIME t_next = std::min({t_int0, t_int1, t_mult, t_ws0, t_ws1});
        if (t_next >= T_END)
            break;

        // ── Collect outputs from all imminent components ──────────────────
        std::optional<double> out_int0, out_int1, out_mult, out_ws0, out_ws1;
        if (t_int0 == t_next)
            out_int0 = get_int(int0.output());
        if (t_int1 == t_next)
            out_int1 = get_int(int1.output());
        if (t_mult == t_next)
            out_mult = get_mult(mult.output());
        if (t_ws0 == t_next)
            out_ws0 = get_ws(ws0.output());
        if (t_ws1 == t_next)
            out_ws1 = get_ws(ws1.output());

        // ── Route outputs to inputs (IC table) ───────────────────────────
        std::optional<double> to_mult_in0, to_mult_in1;
        std::optional<double> to_ws0_in0, to_ws0_in1;
        std::optional<double> to_ws1_in0, to_ws1_in1;
        std::optional<double> to_int0_u, to_int1_u;

        if (out_int0) {
            to_mult_in0 = out_int0;
            to_ws0_in0  = out_int0;
        }
        if (out_int1) {
            to_mult_in1 = out_int1;
            to_ws1_in1  = out_int1;
        }
        if (out_mult) {
            to_ws0_in1 = out_mult;
            to_ws1_in0 = out_mult;
        }
        if (out_ws0)
            to_int0_u = out_ws0;
        if (out_ws1)
            to_int1_u = out_ws1;

        // ── Internal transitions on imminent components ───────────────────
        if (t_int0 == t_next) {
            int0.internal_transition();
            t_int0 = t_next + int0.time_advance();
        }
        if (t_int1 == t_next) {
            int1.internal_transition();
            t_int1 = t_next + int1.time_advance();
        }
        if (t_mult == t_next) {
            mult.internal_transition();
            t_mult = t_next + mult.time_advance();
        }
        if (t_ws0 == t_next) {
            ws0.internal_transition();
            t_ws0 = t_next + ws0.time_advance();
        }
        if (t_ws1 == t_next) {
            ws1.internal_transition();
            t_ws1 = t_next + ws1.time_advance();
        }

        // ── External transitions on components that received messages ─────
        if (to_mult_in0 || to_mult_in1) {
            mult.external_transition(t_next - last_mult, mult_box(to_mult_in0, to_mult_in1));
            last_mult = t_next;
            t_mult    = t_next + mult.time_advance();
        }
        if (to_ws0_in0 || to_ws0_in1) {
            ws0.external_transition(t_next - last_ws0, ws_box(to_ws0_in0, to_ws0_in1));
            last_ws0 = t_next;
            t_ws0    = t_next + ws0.time_advance();
        }
        if (to_ws1_in0 || to_ws1_in1) {
            ws1.external_transition(t_next - last_ws1, ws_box(to_ws1_in0, to_ws1_in1));
            last_ws1 = t_next;
            t_ws1    = t_next + ws1.time_advance();
        }
        if (to_int0_u) {
            int0.external_transition(t_next - last_int0, int_box(*to_int0_u));
            last_int0 = t_next;
            t_int0    = t_next + int0.time_advance();
        }
        if (to_int1_u) {
            int1.external_transition(t_next - last_int1, int_box(*to_int1_u));
            last_int1 = t_next;
            t_int1    = t_next + int1.time_advance();
        }

        // ── Record trajectory at each integrator output event ─────────────
        if (out_int0 || out_int1) {
            std::cout << t_next << "," << int0.state.q << "," << int1.state.q << "\n";
        }
    }

    return 0;
}
