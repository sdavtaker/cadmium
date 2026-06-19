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

#ifndef CADMIUM_PDEVS_ACCUMULATOR_HPP
#define CADMIUM_PDEVS_ACCUMULATOR_HPP

#include <cadmium/modeling/message_bag.hpp>
#include <cadmium/modeling/ports.hpp>

#include <limits>
#include <ostream>
#include <stdexcept>

namespace cadmium::basic_models::pdevs {

    /**
     * @brief Accumulator PDEVS Model.
     *
     * Accumulator PDEVS Model:
     * - In_Ports: add<Numeric>, reset
     * - Out_Ports: outport
     * - S = {Numeric:total, bool:reseted}
     * - internal({total, true}, 0) = {0, false}
     * - external({total, b}, t, x) = {total+x, b}
     * - confluence({total, b}, 0, x) = external(internal({total, true}, 0), 0, x)
     * - output ({total, b}) = outport:{total}
     * - time_advance({total, true}) = 0
     *   time_advance({total, false}) = infinite
     */

    // definitions used for defining the accumulator that need to be accessed by
    // externals resources before instantiate the models
    template <typename VALUE> struct accumulator_defs {
        struct reset_tick {
            friend std::ostream &operator<<(std::ostream &os, const reset_tick &) {
                return os << "reset_tick{}";
            }
        };
        struct sum : public out_port<VALUE> {};
        struct add : public in_port<VALUE> {};
        struct reset : public in_port<reset_tick> {};
    };

    template <typename VALUE, typename TIME> class accumulator {
        using defs = accumulator_defs<VALUE>;

      public:
        struct state_type {
            VALUE sum{};
            bool on_reset = false;

            bool operator==(const state_type &) const = default;

            friend std::ostream &operator<<(std::ostream &os, const state_type &s) {
                return os << "sum=" << s.sum << " on_reset=" << s.on_reset;
            }
        };
        state_type state{};

        constexpr accumulator() noexcept {}

        using input_ports  = std::tuple<typename defs::add, typename defs::reset>;
        using output_ports = std::tuple<typename defs::sum>;

        void internal_transition() {
            if (!state.on_reset)
                throw std::logic_error("Internal transition called while not on reset state");
            state.sum      = VALUE{0};
            state.on_reset = false;
        }

        void external_transition(TIME, typename make_message_bags<input_ports>::type mbs) {
            if (state.on_reset)
                throw std::logic_error("External transition called while on reset state");
            for (const auto &x : get_messages<typename defs::add>(mbs))
                state.sum += x;
            if (!get_messages<typename defs::reset>(mbs).empty())
                state.on_reset = true;
        }

        void confluence_transition(TIME, typename make_message_bags<input_ports>::type mbs) {
            internal_transition();
            external_transition(TIME{}, std::move(mbs));
        }

        typename make_message_bags<output_ports>::type output() const {
            if (!state.on_reset)
                throw std::logic_error("Output function called while not on reset state");
            typename make_message_bags<output_ports>::type outmb;
            get_messages<typename defs::sum>(outmb).emplace_back(state.sum);
            return outmb;
        }

        TIME time_advance() const {
            return state.on_reset ? TIME{} : std::numeric_limits<TIME>::infinity();
        }
    };
} // namespace cadmium::basic_models::pdevs

#endif // CADMIUM_PDEVS_ACCUMULATOR_HPP
