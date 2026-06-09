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
 * STDEVS stochastic channel — coupled simulation example.
 *
 * Models a source transmitting packets over an unreliable channel:
 *
 *   [source] --IC--> [channel] --EOC--> (delivered_out)
 *
 *   source:  Bernoulli generator, period ∈ {1, 3} with p_fast=0.5.
 *            Mean inter-arrival time = 2 time units.
 *   channel: Bernoulli processor, accepts each packet with p=0.8,
 *            forwards after a fixed transmission delay of 0.5 time units.
 *            Drop rate ≈ 20 %.
 *
 * The coupled model is run twice with the same seed (exact replay) and
 * once with a different seed to show trajectory divergence.
 */

#include <cadmium/basic_model/stdevs/bernoulli_generator.hpp>
#include <cadmium/basic_model/stdevs/bernoulli_processor.hpp>
#include <cadmium/engine/stdevs_runner.hpp>
#include <cadmium/modeling/coupling.hpp>
#include <cadmium/modeling/ports.hpp>

#include <chrono>
#include <iostream>
#include <random>

using namespace std;
using hclock = chrono::high_resolution_clock;

using gen_defs  = cadmium::basic_models::stdevs::bernoulli_generator_defs<int>;
using proc_defs = cadmium::basic_models::stdevs::bernoulli_processor_defs<int>;

// ── Concrete submodels ─────────────────────────────────────────────────────

template <typename TIME>
struct packet_source
    : public cadmium::basic_models::stdevs::bernoulli_generator<int, TIME, mt19937> {
    TIME period_a() const override {
        return TIME{1.0f};
    }
    TIME period_b() const override {
        return TIME{3.0f};
    }
    double p() const override {
        return 0.5;
    }
    int output_message() const override {
        return 1;
    }
};

template <typename TIME>
struct lossy_channel
    : public cadmium::basic_models::stdevs::bernoulli_processor<int, TIME, mt19937> {
    TIME processing_time() const override {
        return TIME{0.5f};
    }
    double p_accept() const override {
        return 0.8;
    }
};

// ── Coupled model ──────────────────────────────────────────────────────────

struct delivered_out : public cadmium::out_port<int> {};

using channel_submodels = cadmium::modeling::models_tuple<packet_source, lossy_channel>;
using channel_eics      = std::tuple<>;
using channel_eocs =
    std::tuple<cadmium::modeling::EOC<lossy_channel, proc_defs::out, delivered_out>>;
using channel_ics =
    std::tuple<cadmium::modeling::IC<packet_source, gen_defs::out, lossy_channel, proc_defs::in>>;

template <typename TIME>
using stochastic_channel_model =
    cadmium::modeling::devs::coupling<TIME, std::tuple<>, std::tuple<delivered_out>,
                                      channel_submodels, channel_eics, channel_eocs, channel_ics,
                                      cadmium::engine::devs::first_imminent>;

// ── Main ───────────────────────────────────────────────────────────────────

int main() {
    const float end_time = 1000.0f;
    const auto seed      = mt19937::result_type{2026};

    auto start = hclock::now();

    cadmium::engine::stdevs::runner<float, stochastic_channel_model> r1(0.0f, seed);
    float t1 = r1.run_until(end_time);

    cadmium::engine::stdevs::runner<float, stochastic_channel_model> r2(0.0f, seed);
    float t2 = r2.run_until(end_time);

    cadmium::engine::stdevs::runner<float, stochastic_channel_model> r3(0.0f, seed + 1);
    float t3 = r3.run_until(end_time);

    auto elapsed = chrono::duration_cast<chrono::duration<double>>(hclock::now() - start).count();

    cout << "STDEVS stochastic channel (source: p_fast=0.5 periods {1,3};"
         << " channel: p_accept=0.8 delay=0.5)\n"
         << "  end_time=" << end_time << "  seed=" << seed << "\n"
         << "  run 1 stopped at t=" << t1 << "\n"
         << "  run 2 (same seed) stopped at t=" << t2 << "  ["
         << (t1 == t2 ? "REPRODUCED" : "DIVERGED — unexpected") << "]\n"
         << "  run 3 (seed+" << 1 << ") stopped at t=" << t3 << "\n"
         << "  Simulation took: " << elapsed << " sec\n";

    return t1 == t2 ? 0 : 1;
}
