// SPDX-License-Identifier: BSD-2-Clause
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

#ifndef CADMIUM_PDEVS_GENERATOR_HPP
#define CADMIUM_PDEVS_GENERATOR_HPP

#include <cadmium/modeling/message_bag.hpp>
#include <cadmium/modeling/ports.hpp>

#include <limits>
#include <stdexcept>

namespace cadmium::basic_models::pdevs {

    /**
     * @brief Generator PDEVS Model
     *
     * Generator PDEVS Model(period, outvalue):
     * - X = {}
     * - Y = {outvalue}
     * - S = {passive, active} x Multiples(period)
     * - internal(phase, t) = ("active", period)
     * - external = {}
     * - out ("active", t) = outvalue
     * - advance(phase, t) = period - t
     *
     * CRTP base: Derived must provide:
     *   TIME  period() const
     *   VALUE output_message() const
     */

    template <typename VALUE> struct generator_defs {
        // custom ports
        struct out : public out_port<VALUE> {};
    };

    template <typename Derived, typename VALUE, typename TIME> class generator {
        using defs = generator_defs<VALUE>;

      public:
        constexpr generator() noexcept {}

        // state definition
        using state_type = int;
        state_type state = 0;

        // ports definition
        using input_ports  = std::tuple<>;
        using output_ports = std::tuple<typename defs::out>;

        // internal transition
        void internal_transition() {}

        // external transition
        void external_transition(TIME, typename make_message_bags<input_ports>::type) {
            throw std::logic_error("External transition called in a model with no input ports");
        }

        // confluence transition
        void confluence_transition(TIME, typename make_message_bags<input_ports>::type) {
            throw std::logic_error("Confluence transition called in a model with no input ports");
        }

        // output function — calls Derived::output_message() without virtual dispatch
        typename make_message_bags<output_ports>::type output() const {
            typename make_message_bags<output_ports>::type bags;
            cadmium::get_messages<typename defs::out>(bags).push_back(
                static_cast<const Derived *>(this)->output_message());
            return bags;
        }

        // time_advance function — calls Derived::period() without virtual dispatch
        TIME time_advance() const {
            return static_cast<const Derived *>(this)->period();
        }
    };
} // namespace cadmium::basic_models::pdevs

#endif // CADMIUM_PDEVS_GENERATOR_HPP
