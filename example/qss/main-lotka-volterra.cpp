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
 * The simulation is executed via the standard cadmium runner.
 * The full trajectory is captured in the NDJSON log emitted to stdout:
 *   grep sim_state output.ndjson | jq ...
 */

#include <cadmium/basic_model/qss/qss1_integrator.hpp>
#include <cadmium/basic_model/qss/qss_multiplier.hpp>
#include <cadmium/basic_model/qss/qss_wsum.hpp>
#include <cadmium/engine/devs_engine_helpers.hpp>
#include <cadmium/engine/devs_runner.hpp>
#include <cadmium/logger/cadmium_log.hpp>
#include <cadmium/modeling/coupling.hpp>

#include <tuple>

using namespace cadmium::basic_models::qss;

// ── TIME type ────────────────────────────────────────────────────────────────

using TIME = double;

// ── Distinct submodel wrapper types ──────────────────────────────────────────
//
// cadmium's static coordinator uses the model type as a key to find each
// sub-engine.  Two integrators of the same template class would collide, so
// each logical component gets its own wrapper type.

template <typename T> struct prey_integrator : qss1_integrator<T> {
    prey_integrator() : qss1_integrator<T>(0.5, 1e-6, 1e-3) {}
};

template <typename T> struct predator_integrator : qss1_integrator<T> {
    predator_integrator() : qss1_integrator<T>(0.5, 1e-6, 1e-3) {}
};

// dx1/dt = +0.1*x1 - 0.1*(x1*x2)
template <typename T> struct prey_derivative : qss_wsum<2, T> {
    prey_derivative() : qss_wsum<2, T>({0.1, -0.1}) {}
};

// dx2/dt = +0.1*(x1*x2) - 0.1*x2
template <typename T> struct predator_derivative : qss_wsum<2, T> {
    predator_derivative() : qss_wsum<2, T>({0.1, -0.1}) {}
};

template <typename T> struct lv_product : qss_multiplier<T> {};

// ── Port aliases ─────────────────────────────────────────────────────────────
//
// Use the time-independent defs structs so the same port type works regardless
// of which TIME instantiation is used at runtime.

using int_out_q = qss1_integrator_defs::out_q;
using int_in_u  = qss1_integrator_defs::in_u;
using ws_out    = qss_wsum_defs<2>::out;
using ws_in0    = qss_wsum_defs<2>::template in<0>;
using ws_in1    = qss_wsum_defs<2>::template in<1>;
using mult_out  = qss_multiplier_defs::out;
using mult_in0  = qss_multiplier_defs::in0;
using mult_in1  = qss_multiplier_defs::in1;

// ── Coupled model ─────────────────────────────────────────────────────────────
//
// Coupling (IC table):
//   prey_integrator.out_q     → lv_product.in0
//   prey_integrator.out_q     → prey_derivative.in<0>     (coeff +0.1: +0.1*x1)
//   predator_integrator.out_q → lv_product.in1
//   predator_integrator.out_q → predator_derivative.in<1> (coeff -0.1: -0.1*x2)
//   lv_product.out            → prey_derivative.in<1>     (coeff -0.1: -0.1*x1*x2)
//   lv_product.out            → predator_derivative.in<0> (coeff +0.1: +0.1*x1*x2)
//   prey_derivative.out       → prey_integrator.in_u
//   predator_derivative.out   → predator_integrator.in_u

template <typename T> struct lotka_volterra {
    using input_ports               = std::tuple<>;
    using output_ports              = std::tuple<>;
    using external_input_couplings  = std::tuple<>;
    using external_output_couplings = std::tuple<>;
    using select                    = cadmium::engine::devs::first_imminent;

    template <typename U>
    using models = std::tuple<prey_integrator<U>, predator_integrator<U>, prey_derivative<U>,
                              predator_derivative<U>, lv_product<U>>;

    using internal_couplings = std::tuple<
        cadmium::modeling::IC<prey_integrator, int_out_q, lv_product, mult_in0>,
        cadmium::modeling::IC<prey_integrator, int_out_q, prey_derivative, ws_in0>,
        cadmium::modeling::IC<predator_integrator, int_out_q, lv_product, mult_in1>,
        cadmium::modeling::IC<predator_integrator, int_out_q, predator_derivative, ws_in1>,
        cadmium::modeling::IC<lv_product, mult_out, prey_derivative, ws_in1>,
        cadmium::modeling::IC<lv_product, mult_out, predator_derivative, ws_in0>,
        cadmium::modeling::IC<prey_derivative, ws_out, prey_integrator, int_in_u>,
        cadmium::modeling::IC<predator_derivative, ws_out, predator_integrator, int_in_u>>;
};

// ── Main ──────────────────────────────────────────────────────────────────────

int main() {
    cadmium::log::init();
    cadmium::engine::devs::runner<TIME, lotka_volterra> r{0.0};
    r.run_until(300.0);
    cadmium::log::flush();
}
