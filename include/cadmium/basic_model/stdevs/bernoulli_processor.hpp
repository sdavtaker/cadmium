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

#ifndef CADMIUM_STDEVS_BERNOULLI_PROCESSOR_HPP
#define CADMIUM_STDEVS_BERNOULLI_PROCESSOR_HPP

#include <cadmium/modeling/message_box.hpp>
#include <cadmium/modeling/ports.hpp>

#include <limits>
#include <optional>
#include <random>

namespace cadmium::basic_models::stdevs {

    /**
     * Port definitions for bernoulli_processor.
     */
    template <typename VALUE> struct bernoulli_processor_defs {
        struct in : public cadmium::in_port<VALUE> {};
        struct out : public cadmium::out_port<VALUE> {};
    };

    /**
     * Bernoulli Processor STDEVS Model.
     *
     * Single-server queue with Bernoulli acceptance policy:
     *   - When idle and a message arrives, the message is accepted with
     *     probability p_accept() and processing begins; otherwise it is dropped.
     *   - While busy, incoming messages are dropped (no queueing).
     *   - After processing_time() the job is forwarded and the processor returns
     *     to idle.
     *
     * This models a stochastic server or unreliable channel where each arriving
     * packet is independently lost with probability 1 - p_accept().
     *
     * @tparam VALUE  Type of messages (same type for input and output).
     * @tparam TIME   Simulation time type.
     * @tparam URNG   Uniform random bit generator (default std::mt19937).
     */
    template <typename VALUE, typename TIME, typename URNG = std::mt19937>
    class bernoulli_processor {
        using defs = bernoulli_processor_defs<VALUE>;

      public:
        virtual TIME processing_time() const = 0; // service time for an accepted job
        virtual double p_accept() const      = 0; // probability of accepting an arriving job

        constexpr bernoulli_processor() noexcept {}

        // state_type: nullopt = idle; has_value = busy processing that job.
        struct state_type {
            std::optional<VALUE> job;
        };
        state_type state{};

        using input_ports  = std::tuple<typename defs::in>;
        using output_ports = std::tuple<typename defs::out>;

        void internal_transition(URNG &) {
            state.job = std::nullopt;
        }

        void external_transition(TIME, typename cadmium::make_message_box<input_ports>::type box,
                                 URNG &rng) {
            if (!state.job.has_value()) {
                const auto &msg = cadmium::get_message<typename defs::in>(box);
                if (msg.has_value()) {
                    std::bernoulli_distribution coin(p_accept());
                    if (coin(rng))
                        state.job = msg.value();
                }
            }
            // If busy, arriving messages are silently dropped.
        }

        typename cadmium::make_message_box<output_ports>::type output() const {
            typename cadmium::make_message_box<output_ports>::type box;
            if (state.job.has_value())
                cadmium::get_message<typename defs::out>(box) = state.job.value();
            return box;
        }

        TIME time_advance() const {
            return state.job.has_value() ? processing_time()
                                         : std::numeric_limits<TIME>::infinity();
        }

        virtual ~bernoulli_processor() {}
    };

} // namespace cadmium::basic_models::stdevs

#endif // CADMIUM_STDEVS_BERNOULLI_PROCESSOR_HPP
