/**
 * Copyright (c) 2013-present, Damian Vicino
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

#include <cadmium/basic_model/devs/generator.hpp>
#include <cadmium/modeling/message_box.hpp>
#include <cadmium/modeling/ports.hpp>

#include <algorithm>
#include <chrono>
#include <iostream>
#include <limits>

using namespace std;
using hclock = chrono::high_resolution_clock;

/**
 * Classic DEVS clock with 3 generators (H, M, S).
 * No DEVS coupled-model runner exists yet; driven by a bespoke SELECT loop.
 * Each generator fires independently; SELECT priority is S > M > H (least frequent last).
 *
 * The experiment runtime is measured using the chrono library.
 */

struct tick {};

using out_p = cadmium::basic_models::devs::generator_defs<tick>::out;

template <typename TIME>
struct hour_generator : public cadmium::basic_models::devs::generator<tick, TIME> {
    TIME period() const override {
        return TIME{3600};
    }
    tick output_message() const override {
        return tick{};
    }
};

template <typename TIME>
struct minute_generator : public cadmium::basic_models::devs::generator<tick, TIME> {
    TIME period() const override {
        return TIME{60};
    }
    tick output_message() const override {
        return tick{};
    }
};

template <typename TIME>
struct second_generator : public cadmium::basic_models::devs::generator<tick, TIME> {
    TIME period() const override {
        return TIME{1};
    }
    tick output_message() const override {
        return tick{};
    }
};

int main() {
    auto start = hclock::now();

    hour_generator<float> H;
    minute_generator<float> M;
    second_generator<float> S;

    float sched_H = H.time_advance();
    float sched_M = M.time_advance();
    float sched_S = S.time_advance();

    long long h_ticks = 0, m_ticks = 0, s_ticks = 0;

    const float end_time = 30000.0f;

    while (true) {
        float t_next = std::min({sched_H, sched_M, sched_S});
        if (t_next >= end_time)
            break;

        // SELECT: S > M > H (most frequent first)
        if (sched_S == t_next) {
            S.output();
            S.internal_transition();
            sched_S = t_next + S.time_advance();
            ++s_ticks;
        } else if (sched_M == t_next) {
            M.output();
            M.internal_transition();
            sched_M = t_next + M.time_advance();
            ++m_ticks;
        } else {
            H.output();
            H.internal_transition();
            sched_H = t_next + H.time_advance();
            ++h_ticks;
        }
    }

    auto elapsed = chrono::duration_cast<chrono::duration<double>>(hclock::now() - start).count();

    cout << "Classic DEVS clock (SELECT: S > M > H)\n"
         << "  end_time=" << end_time << "s\n"
         << "  S ticks: " << s_ticks << "  (expected " << (long long)end_time << ")\n"
         << "  M ticks: " << m_ticks << "  (expected " << (long long)(end_time / 60) << ")\n"
         << "  H ticks: " << h_ticks << "  (expected " << (long long)(end_time / 3600) << ")\n"
         << "  Simulation took: " << elapsed << " sec\n";

    return 0;
}
