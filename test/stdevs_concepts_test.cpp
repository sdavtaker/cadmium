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

/**
 * Concept-satisfaction tests for the STDEVS modeling language.
 *
 * Every check is a STATIC_REQUIRE so failures are caught at compile time and
 * reported as normal Catch2 output.  The fixture structs below each violate
 * exactly one requirement so that the negative tests remain focused.
 */

#include <cadmium/concepts/devs_concepts.hpp>
#include <cadmium/concepts/stdevs_concepts.hpp>
#include <cadmium/modeling/message_box.hpp>
#include <cadmium/modeling/ports.hpp>

#include <catch2/catch_test_macros.hpp>
#include <random>
#include <tuple>

namespace {

    using cadmium::concepts::stdevs::AtomicModel;
    using cadmium::concepts::stdevs::CoupledModel;

    using RNG = std::mt19937;

    // ═══════════════════════════════════════════════════════════════════════════
    // Valid atomic models
    //
    // valid_stdevs is templated on URNG so the same fixture can verify the
    // concept works with any uniform_random_bit_generator, not just mt19937.
    // ═══════════════════════════════════════════════════════════════════════════

    template <typename TIME, typename URNG = RNG> struct valid_stdevs {
        struct in : public cadmium::in_port<int> {};
        struct out : public cadmium::out_port<int> {};
        using state_type   = int;
        state_type state   = 0;
        using input_ports  = std::tuple<in>;
        using output_ports = std::tuple<out>;

        void internal_transition(URNG &) {}
        void external_transition(TIME, typename cadmium::make_message_box<input_ports>::type,
                                 URNG &) {}
        typename cadmium::make_message_box<output_ports>::type output() const {
            return {};
        }
        TIME time_advance() const {
            return TIME{};
        }
    };

    template <typename TIME, typename URNG = RNG> struct valid_stdevs_multi_port {
        struct in1 : public cadmium::in_port<int> {};
        struct in2 : public cadmium::in_port<float> {};
        struct out1 : public cadmium::out_port<int> {};
        struct out2 : public cadmium::out_port<float> {};
        using state_type   = int;
        state_type state   = 0;
        using input_ports  = std::tuple<in1, in2>;
        using output_ports = std::tuple<out1, out2>;

        void internal_transition(URNG &) {}
        void external_transition(TIME, typename cadmium::make_message_box<input_ports>::type,
                                 URNG &) {}
        typename cadmium::make_message_box<output_ports>::type output() const {
            return {};
        }
        TIME time_advance() const {
            return TIME{};
        }
    };

    // ═══════════════════════════════════════════════════════════════════════════
    // Invalid atomic models — one violation each
    // ═══════════════════════════════════════════════════════════════════════════

    // missing state_type typedef
    template <typename TIME> struct no_state_type {
        struct in : public cadmium::in_port<int> {};
        struct out : public cadmium::out_port<int> {};
        int state          = 0;
        using input_ports  = std::tuple<in>;
        using output_ports = std::tuple<out>;
        void internal_transition(RNG &) {}
        void external_transition(TIME, typename cadmium::make_message_box<input_ports>::type,
                                 RNG &) {}
        typename cadmium::make_message_box<output_ports>::type output() const {
            return {};
        }
        TIME time_advance() const {
            return TIME{};
        }
    };

    // missing state variable
    template <typename TIME> struct no_state_var {
        struct in : public cadmium::in_port<int> {};
        struct out : public cadmium::out_port<int> {};
        using state_type   = int;
        using input_ports  = std::tuple<in>;
        using output_ports = std::tuple<out>;
        void internal_transition(RNG &) {}
        void external_transition(TIME, typename cadmium::make_message_box<input_ports>::type,
                                 RNG &) {}
        typename cadmium::make_message_box<output_ports>::type output() const {
            return {};
        }
        TIME time_advance() const {
            return TIME{};
        }
    };

    // internal_transition takes no arguments (DEVS signature, not STDEVS)
    template <typename TIME> struct internal_no_rng {
        struct in : public cadmium::in_port<int> {};
        struct out : public cadmium::out_port<int> {};
        using state_type   = int;
        state_type state   = 0;
        using input_ports  = std::tuple<in>;
        using output_ports = std::tuple<out>;
        void internal_transition() {} // wrong: no rng
        void external_transition(TIME, typename cadmium::make_message_box<input_ports>::type,
                                 RNG &) {}
        typename cadmium::make_message_box<output_ports>::type output() const {
            return {};
        }
        TIME time_advance() const {
            return TIME{};
        }
    };

    // external_transition missing rng (DEVS signature, not STDEVS)
    template <typename TIME> struct external_no_rng {
        struct in : public cadmium::in_port<int> {};
        struct out : public cadmium::out_port<int> {};
        using state_type   = int;
        state_type state   = 0;
        using input_ports  = std::tuple<in>;
        using output_ports = std::tuple<out>;
        void internal_transition(RNG &) {}
        void external_transition(TIME, typename cadmium::make_message_box<input_ports>::type) {
        } // wrong: no rng
        typename cadmium::make_message_box<output_ports>::type output() const {
            return {};
        }
        TIME time_advance() const {
            return TIME{};
        }
    };

    // internal_transition missing entirely
    template <typename TIME> struct no_internal_transition {
        struct in : public cadmium::in_port<int> {};
        struct out : public cadmium::out_port<int> {};
        using state_type   = int;
        state_type state   = 0;
        using input_ports  = std::tuple<in>;
        using output_ports = std::tuple<out>;
        void external_transition(TIME, typename cadmium::make_message_box<input_ports>::type,
                                 RNG &) {}
        typename cadmium::make_message_box<output_ports>::type output() const {
            return {};
        }
        TIME time_advance() const {
            return TIME{};
        }
    };

    // external_transition missing entirely
    template <typename TIME> struct no_external_transition {
        struct in : public cadmium::in_port<int> {};
        struct out : public cadmium::out_port<int> {};
        using state_type   = int;
        state_type state   = 0;
        using input_ports  = std::tuple<in>;
        using output_ports = std::tuple<out>;
        void internal_transition(RNG &) {}
        typename cadmium::make_message_box<output_ports>::type output() const {
            return {};
        }
        TIME time_advance() const {
            return TIME{};
        }
    };

    // output() missing
    template <typename TIME> struct no_output {
        struct in : public cadmium::in_port<int> {};
        struct out : public cadmium::out_port<int> {};
        using state_type   = int;
        state_type state   = 0;
        using input_ports  = std::tuple<in>;
        using output_ports = std::tuple<out>;
        void internal_transition(RNG &) {}
        void external_transition(TIME, typename cadmium::make_message_box<input_ports>::type,
                                 RNG &) {}
        TIME time_advance() const {
            return TIME{};
        }
    };

    // time_advance() missing
    template <typename TIME> struct no_time_advance {
        struct in : public cadmium::in_port<int> {};
        struct out : public cadmium::out_port<int> {};
        using state_type   = int;
        state_type state   = 0;
        using input_ports  = std::tuple<in>;
        using output_ports = std::tuple<out>;
        void internal_transition(RNG &) {}
        void external_transition(TIME, typename cadmium::make_message_box<input_ports>::type,
                                 RNG &) {}
        typename cadmium::make_message_box<output_ports>::type output() const {
            return {};
        }
    };

    // duplicate input port type
    template <typename TIME> struct repeated_input_port {
        struct in1 : public cadmium::in_port<int> {};
        struct out : public cadmium::out_port<int> {};
        using state_type   = int;
        state_type state   = 0;
        using input_ports  = std::tuple<in1, in1>;
        using output_ports = std::tuple<out>;
        void internal_transition(RNG &) {}
        void external_transition(TIME, typename cadmium::make_message_box<input_ports>::type,
                                 RNG &) {}
        typename cadmium::make_message_box<output_ports>::type output() const {
            return {};
        }
        TIME time_advance() const {
            return TIME{};
        }
    };

    // duplicate output port type
    template <typename TIME> struct repeated_output_port {
        struct in : public cadmium::in_port<int> {};
        struct out : public cadmium::out_port<int> {};
        using state_type   = int;
        state_type state   = 0;
        using input_ports  = std::tuple<in>;
        using output_ports = std::tuple<out, out>;
        void internal_transition(RNG &) {}
        void external_transition(TIME, typename cadmium::make_message_box<input_ports>::type,
                                 RNG &) {}
        typename cadmium::make_message_box<output_ports>::type output() const {
            return {};
        }
        TIME time_advance() const {
            return TIME{};
        }
    };

    // ═══════════════════════════════════════════════════════════════════════════
    // Non-URNG type for URNG-constraint test
    // ═══════════════════════════════════════════════════════════════════════════

    struct not_a_rng {}; // does not satisfy std::uniform_random_bit_generator

} // anonymous namespace

// ═══════════════════════════════════════════════════════════════════════════
// AtomicModel concept — positive
// ═══════════════════════════════════════════════════════════════════════════

SCENARIO("minimal valid STDEVS atomic model satisfies AtomicModel",
         "[stdevs][concepts][atomic][positive]") {
    STATIC_REQUIRE(AtomicModel<valid_stdevs<float>, float, RNG>);
}

SCENARIO("valid STDEVS atomic with multiple ports satisfies AtomicModel",
         "[stdevs][concepts][atomic][positive]") {
    STATIC_REQUIRE(AtomicModel<valid_stdevs_multi_port<float>, float, RNG>);
}

SCENARIO("model templated on minstd_rand satisfies AtomicModel with minstd_rand",
         "[stdevs][concepts][atomic][positive]") {
    STATIC_REQUIRE(AtomicModel<valid_stdevs<float, std::minstd_rand>, float, std::minstd_rand>);
}

// ═══════════════════════════════════════════════════════════════════════════
// AtomicModel concept — negative
// ═══════════════════════════════════════════════════════════════════════════

SCENARIO("model missing state_type does not satisfy AtomicModel",
         "[stdevs][concepts][atomic][negative]") {
    STATIC_REQUIRE(!AtomicModel<no_state_type<float>, float, RNG>);
}

SCENARIO("model missing state variable does not satisfy AtomicModel",
         "[stdevs][concepts][atomic][negative]") {
    STATIC_REQUIRE(!AtomicModel<no_state_var<float>, float, RNG>);
}

SCENARIO("internal_transition without rng does not satisfy AtomicModel",
         "[stdevs][concepts][atomic][negative]") {
    STATIC_REQUIRE(!AtomicModel<internal_no_rng<float>, float, RNG>);
}

SCENARIO("external_transition without rng does not satisfy AtomicModel",
         "[stdevs][concepts][atomic][negative]") {
    STATIC_REQUIRE(!AtomicModel<external_no_rng<float>, float, RNG>);
}

SCENARIO("model missing internal_transition does not satisfy AtomicModel",
         "[stdevs][concepts][atomic][negative]") {
    STATIC_REQUIRE(!AtomicModel<no_internal_transition<float>, float, RNG>);
}

SCENARIO("model missing external_transition does not satisfy AtomicModel",
         "[stdevs][concepts][atomic][negative]") {
    STATIC_REQUIRE(!AtomicModel<no_external_transition<float>, float, RNG>);
}

SCENARIO("model missing output() does not satisfy AtomicModel",
         "[stdevs][concepts][atomic][negative]") {
    STATIC_REQUIRE(!AtomicModel<no_output<float>, float, RNG>);
}

SCENARIO("model missing time_advance() does not satisfy AtomicModel",
         "[stdevs][concepts][atomic][negative]") {
    STATIC_REQUIRE(!AtomicModel<no_time_advance<float>, float, RNG>);
}

SCENARIO("model with repeated input port type does not satisfy AtomicModel",
         "[stdevs][concepts][atomic][negative]") {
    STATIC_REQUIRE(!AtomicModel<repeated_input_port<float>, float, RNG>);
}

SCENARIO("model with repeated output port type does not satisfy AtomicModel",
         "[stdevs][concepts][atomic][negative]") {
    STATIC_REQUIRE(!AtomicModel<repeated_output_port<float>, float, RNG>);
}

SCENARIO("model and URNG type mismatch does not satisfy AtomicModel",
         "[stdevs][concepts][atomic][negative]") {
    // valid_stdevs<float> takes std::mt19937& — passing not_a_rng fails the requires
    // clause because not_a_rng& cannot bind to the model's std::mt19937& parameter.
    STATIC_REQUIRE(!AtomicModel<valid_stdevs<float>, float, not_a_rng>);
}

// ═══════════════════════════════════════════════════════════════════════════
// CoupledModel concept — stdevs::CoupledModel aliases devs::CoupledModel
// ═══════════════════════════════════════════════════════════════════════════

SCENARIO("stdevs::CoupledModel accepts any devs::CoupledModel",
         "[stdevs][concepts][coupled][positive]") {
    // A valid_stdevs atomic does not satisfy CoupledModel (no coupling members)
    STATIC_REQUIRE(!CoupledModel<valid_stdevs<float>>);
}

SCENARIO("plain STDEVS atomic does not satisfy CoupledModel",
         "[stdevs][concepts][coupled][negative]") {
    STATIC_REQUIRE(!CoupledModel<valid_stdevs<float>>);
}
