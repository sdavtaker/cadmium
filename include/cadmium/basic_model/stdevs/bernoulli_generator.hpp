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

#ifndef CADMIUM_STDEVS_BERNOULLI_GENERATOR_HPP
#define CADMIUM_STDEVS_BERNOULLI_GENERATOR_HPP

#include <cadmium/modeling/message_box.hpp>
#include <cadmium/modeling/ports.hpp>

#include <ostream>
#include <random>
#include <stdexcept>

namespace cadmium::basic_models::stdevs {

    /**
     * Port definitions for bernoulli_generator, separated so couplings can
     * reference them before the model template is instantiated (same pattern
     * as devs/pdevs generator_defs).
     */
    template <typename VALUE> struct bernoulli_generator_defs {
        struct out : public cadmium::out_port<VALUE> {};
    };

    /**
     * Bernoulli Generator STDEVS Model.
     *
     * Generates a stream of output messages where each inter-event interval
     * is drawn from a two-outcome Bernoulli distribution:
     *   - period_a()  with probability  p()
     *   - period_b()  with probability  1 - p()
     *
     * The first event fires at time period_a() (deterministic).  Subclass and
     * override period_a(), period_b(), p(), and output_message() to configure.
     *
     * This models the stochastic source used in the LBM Load Generator (CKW10
     * §7.2) when restricted to a two-valued period distribution.
     *
     * @tparam VALUE  Type of the output message.
     * @tparam TIME   Simulation time type.
     * @tparam URNG   Uniform random bit generator (default std::mt19937).
     */
    template <typename VALUE, typename TIME, typename URNG = std::mt19937>
    class bernoulli_generator {
        using defs = bernoulli_generator_defs<VALUE>;

      public:
        virtual TIME period_a() const        = 0; // period chosen with probability p()
        virtual TIME period_b() const        = 0; // period chosen with probability 1 - p()
        virtual double p() const             = 0; // Bernoulli success probability
        virtual VALUE output_message() const = 0;

        constexpr bernoulli_generator() noexcept {}

        // state_type holds the current inter-event interval.
        // `initialized` is false until the first internal transition; before that,
        // time_advance() returns period_a() by calling the virtual directly (we
        // cannot store the result at construction because virtuals are not yet
        // dispatched to the derived class).
        struct state_type {
            TIME sigma{};
            bool initialized = false;

            friend std::ostream &operator<<(std::ostream &os, const state_type &s) {
                return os << "sigma=" << s.sigma << " initialized=" << s.initialized;
            }
        };
        state_type state{};

        using input_ports  = std::tuple<>;
        using output_ports = std::tuple<typename defs::out>;

        void internal_transition(URNG &rng) {
            std::bernoulli_distribution coin(p());
            state.sigma       = coin(rng) ? period_a() : period_b();
            state.initialized = true;
        }

        void external_transition(TIME, typename cadmium::make_message_box<input_ports>::type,
                                 URNG &) {
            throw std::logic_error("External transition called in a model with no input ports");
        }

        typename cadmium::make_message_box<output_ports>::type output() const {
            typename cadmium::make_message_box<output_ports>::type box;
            cadmium::get_message<typename defs::out>(box).emplace(output_message());
            return box;
        }

        TIME time_advance() const {
            return state.initialized ? state.sigma : period_a();
        }

        virtual ~bernoulli_generator() {}
    };

} // namespace cadmium::basic_models::stdevs

#endif // CADMIUM_STDEVS_BERNOULLI_GENERATOR_HPP
