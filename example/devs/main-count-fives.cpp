/**
 * Copyright (c) 2017-present, Damian Vicino
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

#include <cadmium/basic_model/devs/accumulator.hpp>
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
 * Classic DEVS count-fives: integer generator every 1s feeds an accumulator
 * which is reset every 5s. Expected output each reset: 5.
 *
 * No DEVS coupled-model runner exists yet; driven by a bespoke SELECT loop.
 * SELECT priority: int_gen > reset_gen > accumulator.
 * Placing int_gen before reset_gen ensures the 5th tick is counted before the
 * reset fires when both are imminent at multiples of 5.
 */

using acc_defs   = cadmium::basic_models::devs::accumulator_defs<int>;
using reset_tick = acc_defs::reset_tick;
using acc_out_p  = cadmium::basic_models::devs::generator_defs<int>::out;

template <typename TIME> struct int_gen : public cadmium::basic_models::devs::generator<int, TIME> {
    TIME period() const override {
        return TIME{1};
    }
    int output_message() const override {
        return 1;
    }
};

template <typename TIME>
struct reset_gen : public cadmium::basic_models::devs::generator<reset_tick, TIME> {
    TIME period() const override {
        return TIME{5};
    }
    reset_tick output_message() const override {
        return reset_tick{};
    }
};

int main() {
    using acc_t = cadmium::basic_models::devs::accumulator<int, float>;

    auto start = hclock::now();

    int_gen<float> ig;
    reset_gen<float> rg;
    acc_t acc;

    const float INF      = numeric_limits<float>::infinity();
    const float end_time = 100.0f;

    float sched_ig  = ig.time_advance();
    float sched_rg  = rg.time_advance();
    float sched_acc = INF;
    float last_acc  = 0.0f;

    long long outputs = 0, errors = 0;

    while (true) {
        float t_next = min({sched_ig, sched_rg, sched_acc});
        if (t_next >= end_time)
            break;

        // SELECT: int_gen > reset_gen > accumulator
        if (sched_ig == t_next) {
            auto out = ig.output();
            ig.internal_transition();
            sched_ig = t_next + ig.time_advance();

            typename cadmium::make_message_box<acc_t::input_ports>::type inbox{};
            cadmium::get_message<acc_defs::add>(inbox).emplace(
                cadmium::get_message<acc_out_p>(out).value());
            acc.external_transition(t_next - last_acc, inbox);
            last_acc  = t_next;
            sched_acc = t_next + acc.time_advance();

        } else if (sched_rg == t_next) {
            rg.output();
            rg.internal_transition();
            sched_rg = t_next + rg.time_advance();

            typename cadmium::make_message_box<acc_t::input_ports>::type inbox{};
            cadmium::get_message<acc_defs::reset>(inbox).emplace(reset_tick{});
            acc.external_transition(t_next - last_acc, inbox);
            last_acc  = t_next;
            sched_acc = t_next + acc.time_advance();

        } else {
            auto out = acc.output();
            acc.internal_transition();
            last_acc  = t_next;
            sched_acc = INF;

            if (auto v = cadmium::get_message<acc_defs::sum>(out)) {
                ++outputs;
                if (*v != 5)
                    ++errors;
                cout << "  t=" << t_next << "  count=" << *v << "\n";
            }
        }
    }

    auto elapsed = chrono::duration_cast<chrono::duration<double>>(hclock::now() - start).count();

    cout << "Classic DEVS count-fives (SELECT: int > reset > acc)\n"
         << "  outputs=" << outputs << "  errors=" << errors << "\n"
         << "  Simulation took: " << elapsed << " sec\n";

    return 0;
}
