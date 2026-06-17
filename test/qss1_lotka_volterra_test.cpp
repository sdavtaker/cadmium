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
 * Lotka-Volterra QSS1 integration test.
 *
 * Runs the same manual PDEVS simulation as main-lotka-volterra.cpp for
 * t in [0, 50] and checks:
 *   1. Both populations remain strictly positive.
 *   2. The conserved quantity V = x1 - ln(x1) + x2 - ln(x2) stays within
 *      a tolerance of the initial value (QSS1 is O(dq) accurate).
 *   3. Event count is in a reasonable range (not Zeno, not stalled).
 *   4. Both populations oscillate: their max exceeds their initial value.
 */

#include <cadmium/basic_model/qss/qss1_integrator.hpp>
#include <cadmium/basic_model/qss/qss_multiplier.hpp>
#include <cadmium/basic_model/qss/qss_wsum.hpp>

#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <cmath>
#include <limits>
#include <optional>

using namespace cadmium::basic_models::qss;
using cadmium::get_message;
using cadmium::make_message_box;
using Catch::Matchers::WithinAbs;

using TIME       = double;
using Integrator = qss1_integrator<TIME>;
using WSum2      = qss_wsum<2, TIME>;
using Mult       = qss_multiplier<TIME>;

// ── Helpers (same as main-lotka-volterra.cpp) ────────────────────────────────

static double get_int(const make_message_box<Integrator::output_ports>::type &b) {
    return get_message<Integrator::out_q>(b).value();
}
static double get_ws(const make_message_box<WSum2::output_ports>::type &b) {
    return get_message<WSum2::out>(b).value();
}
static double get_mult_val(const make_message_box<Mult::output_ports>::type &b) {
    return get_message<Mult::out>(b).value();
}

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

// Lotka-Volterra conserved quantity V = x1 - ln(x1) + x2 - ln(x2)
static double conserved(double x1, double x2) {
    return x1 - std::log(x1) + x2 - std::log(x2);
}

// ── Single shared simulation run ─────────────────────────────────────────────

struct LVResult {
    long long events;
    double min_x1, max_x1;
    double min_x2, max_x2;
    double max_V_drift; // max |V(t) - V(0)|
};

static LVResult run_lv(double t_end, double dqmin, double dqrel) {
    constexpr TIME INF = std::numeric_limits<TIME>::infinity();

    Integrator int0(0.5, dqmin, dqrel);
    Integrator int1(0.5, dqmin, dqrel);
    Mult mult{};
    WSum2 ws0({0.1, -0.1});
    WSum2 ws1({0.1, -0.1});

    TIME t_int0 = int0.time_advance(), t_int1 = int1.time_advance();
    TIME t_mult = INF, t_ws0 = INF, t_ws1 = INF;
    TIME last_int0 = 0, last_int1 = 0, last_mult = 0, last_ws0 = 0, last_ws1 = 0;

    const double V0 = conserved(0.5, 0.5);
    LVResult r{};
    r.min_x1 = r.min_x2 = 0.5;
    r.max_x1 = r.max_x2 = 0.5;
    r.max_V_drift       = 0.0;

    while (true) {
        TIME t_next = std::min({t_int0, t_int1, t_mult, t_ws0, t_ws1});
        if (t_next >= t_end)
            break;
        ++r.events;

        std::optional<double> out_int0, out_int1, out_mult, out_ws0, out_ws1;
        if (t_int0 == t_next)
            out_int0 = get_int(int0.output());
        if (t_int1 == t_next)
            out_int1 = get_int(int1.output());
        if (t_mult == t_next)
            out_mult = get_mult_val(mult.output());
        if (t_ws0 == t_next)
            out_ws0 = get_ws(ws0.output());
        if (t_ws1 == t_next)
            out_ws1 = get_ws(ws1.output());

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
        if (out_ws0) {
            to_int0_u = out_ws0;
        }
        if (out_ws1) {
            to_int1_u = out_ws1;
        }

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

        // Track extremes and conserved quantity at integrator events
        if (out_int0 || out_int1) {
            double x1 = int0.state.q, x2 = int1.state.q;
            r.min_x1 = std::min(r.min_x1, x1);
            r.max_x1 = std::max(r.max_x1, x1);
            r.min_x2 = std::min(r.min_x2, x2);
            r.max_x2 = std::max(r.max_x2, x2);
            if (x1 > 0.0 && x2 > 0.0)
                r.max_V_drift = std::max(r.max_V_drift, std::abs(conserved(x1, x2) - V0));
        }
    }
    return r;
}

// ── Tests ─────────────────────────────────────────────────────────────────────

TEST_CASE("lotka-volterra: both populations remain positive", "[qss1][lv]") {
    auto r = run_lv(50.0, 1e-6, 1e-3);
    REQUIRE(r.min_x1 > 0.0);
    REQUIRE(r.min_x2 > 0.0);
}

TEST_CASE("lotka-volterra: populations oscillate beyond initial value", "[qss1][lv]") {
    auto r = run_lv(50.0, 1e-6, 1e-3);
    // Both populations must exceed 0.5 at some point (oscillation, not decay)
    REQUIRE(r.max_x1 > 0.5);
    REQUIRE(r.max_x2 > 0.5);
}

TEST_CASE("lotka-volterra: conserved quantity drift is small over t=50", "[qss1][lv]") {
    // QSS1 is O(dq) accurate; drift accumulates over time but should stay << 1
    auto r = run_lv(50.0, 1e-6, 1e-3);
    REQUIRE(r.max_V_drift < 0.2);
}

TEST_CASE("lotka-volterra: event count is in reasonable range", "[qss1][lv]") {
    auto r = run_lv(50.0, 1e-6, 1e-3);
    // Should produce thousands of events, not millions (not Zeno) and not just a handful
    REQUIRE(r.events > 100);
    REQUIRE(r.events < 5'000'000);
}

TEST_CASE("lotka-volterra: tighter quantum gives smaller conserved drift", "[qss1][lv]") {
    auto r_coarse = run_lv(20.0, 1e-3, 0.0);
    auto r_fine   = run_lv(20.0, 1e-4, 0.0);
    // Finer quantum should give less drift (QSS1 convergence property)
    REQUIRE(r_fine.max_V_drift < r_coarse.max_V_drift);
}
