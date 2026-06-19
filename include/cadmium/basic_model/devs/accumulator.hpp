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

#ifndef CADMIUM_DEVS_ACCUMULATOR_HPP
#define CADMIUM_DEVS_ACCUMULATOR_HPP

#include <cadmium/modeling/message_box.hpp>
#include <cadmium/modeling/ports.hpp>

#include <limits>
#include <ostream>
#include <stdexcept>

namespace cadmium::basic_models::devs {

    /**
     * @brief Accumulator DEVS Model.
     *
     * Accumulator DEVS Model:
     * - In_Ports: add<Numeric>, reset
     * - Out_Ports: outport
     * - S = {Numeric:total, bool:reseted}
     * - internal({total, true}, 0) = {0, false}
     * - external({total, b}, t, x) = {total+x, b}
     * - output ({total, b}) = outport:{total}
     * - time_advance({total, true}) = 0
     *   time_advance({total, false}) = infinite
     */

    // definitions used for defining the accumulator that need to be accessed by
    // externals resources before instantiate the models
    template <typename VALUE> struct accumulator_defs {
        // custom messages
        struct reset_tick {
            friend std::ostream &operator<<(std::ostream &os, const reset_tick &) {
                return os << "reset_tick{}";
            }
        };
        // custom ports
        struct add : public in_port<VALUE> {};
        struct reset : public in_port<reset_tick> {};
        struct sum : public out_port<VALUE> {};
    };

    template <typename VALUE,
              typename TIME> // value is the type of accumulated values
    class accumulator {
        using defs = accumulator_defs<VALUE>; // putting definitions in context
      public:
        // state
        struct state_type {
            VALUE sum{};
            bool on_reset = false;

            friend std::ostream &operator<<(std::ostream &os, const state_type &s) {
                return os << "sum=" << s.sum << " on_reset=" << s.on_reset;
            }
        };
        state_type state{};

        // default constructor
        constexpr accumulator() noexcept {}

        // ports_definition
        using input_ports  = std::tuple<typename defs::add, typename defs::reset>;
        using output_ports = std::tuple<typename defs::sum>;

        // DEVS functions
        void internal_transition() {
            if (!state.on_reset) {
                throw std::logic_error("Internal transition called while not on reset state");
            }
            state.sum      = VALUE{0};
            state.on_reset = false;
        }

        void external_transition(TIME, typename make_message_box<input_ports>::type mb) {
            if (state.on_reset) {
                throw std::logic_error("External transition called while on reset state");
            }

            auto &adder = get_message<typename defs::add>(mb);
            if (adder.has_value()) {
                state.sum += adder.value();
            }
            if (get_message<typename defs::reset>(mb).has_value()) {
                state.on_reset = true;
            }
        }

        typename make_message_box<output_ports>::type output() const {
            if (!state.on_reset) {
                throw std::logic_error("Output function called while not on reset state");
            }

            typename make_message_box<output_ports>::type outmb;
            get_message<typename defs::sum>(outmb).emplace(state.sum);
            return outmb;
        }

        TIME time_advance() const {
            return (state.on_reset ? TIME{} : std::numeric_limits<TIME>::infinity());
        }
    };
} // namespace cadmium::basic_models::devs

#endif // CADMIUM_DEVS_ACCUMULATOR_HPP
