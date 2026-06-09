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
 * STDEVS Bernoulli generator — atomic simulation example.
 *
 * Demonstrates the key property of STDEVS simulations: exact replay.
 * Two runners seeded identically produce the same event stream; runners
 * seeded differently diverge.  The seed is logged by the runner so any
 * run can be reproduced exactly.
 *
 * Model: stochastic event source with period drawn from Bernoulli(p=0.7):
 *   fast period (0.5 time units) with probability 0.7
 *   slow period (2.0 time units) with probability 0.3
 * Expected mean inter-arrival time: 0.7*0.5 + 0.3*2.0 = 0.95 time units.
 */

#include <cadmium/basic_model/stdevs/bernoulli_generator.hpp>
#include <cadmium/engine/stdevs_runner.hpp>

#include <chrono>
#include <iostream>
#include <random>

using namespace std;
using hclock = chrono::high_resolution_clock;

using gen_defs = cadmium::basic_models::stdevs::bernoulli_generator_defs<int>;

template <typename TIME>
struct event_source
    : public cadmium::basic_models::stdevs::bernoulli_generator<int, TIME, mt19937> {
    TIME period_a() const override {
        return TIME{0.5f};
    } // fast — probability p()
    TIME period_b() const override {
        return TIME{2.0f};
    } // slow — probability 1-p()
    double p() const override {
        return 0.7;
    }
    int output_message() const override {
        return 1;
    }
};

int main() {
    const float end_time = 1000.0f;
    const auto seed      = mt19937::result_type{42};

    auto start = hclock::now();

    // First run with explicit seed.
    cadmium::engine::stdevs::runner<float, event_source> r1(0.0f, seed);
    float t1 = r1.run_until(end_time);

    // Replay: same seed must produce exactly the same trajectory.
    cadmium::engine::stdevs::runner<float, event_source> r2(0.0f, seed);
    float t2 = r2.run_until(end_time);

    // Different seed: trajectory diverges (almost surely).
    cadmium::engine::stdevs::runner<float, event_source> r3(0.0f, seed + 1);
    float t3 = r3.run_until(end_time);

    auto elapsed = chrono::duration_cast<chrono::duration<double>>(hclock::now() - start).count();

    cout << "STDEVS Bernoulli generator (p_fast=0.7, fast=0.5, slow=2.0)\n"
         << "  end_time=" << end_time << "  seed=" << seed << "\n"
         << "  run 1 stopped at t=" << t1 << "\n"
         << "  run 2 (same seed) stopped at t=" << t2 << "  ["
         << (t1 == t2 ? "REPRODUCED" : "DIVERGED — unexpected") << "]\n"
         << "  run 3 (seed+" << 1 << ") stopped at t=" << t3 << "\n"
         << "  Simulation took: " << elapsed << " sec\n";

    return t1 == t2 ? 0 : 1;
}
