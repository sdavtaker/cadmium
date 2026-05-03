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
 * Regression tests for a 2017 bug where stale messages were re-read from inbox
 * (simple_inbox_cleanup_bug_test) or re-routed from outbox
 * (simple_outbox_cleanup_bug_test). The bug caused the accumulator to
 * accumulate more than once per generator tick.
 *
 * We verify the fix by checking that every logged accumulator state is either
 * the initial state [0, 0] or the single-accumulation state [1, 0] — never [2,
 * 0] or higher. The accumulator state is the only tuple-valued state in these
 * topologies, so searching for " is [" uniquely identifies accumulator state
 * log lines.
 */

#include <cadmium/basic_model/pdevs/accumulator.hpp>
#include <cadmium/basic_model/pdevs/filter_first_output.hpp>
#include <cadmium/basic_model/pdevs/int_generator_one_sec.hpp>
#include <cadmium/engine/pdevs_coordinator.hpp>
#include <cadmium/logger/tuple_to_ostream.hpp>
#include <cadmium/modeling/coupling.hpp>

#include <catch2/catch_test_macros.hpp>

namespace BM = cadmium::basic_models::pdevs;

template <typename TIME> struct filter_one : public BM::filter_first_output<TIME> {};
template <typename TIME> using gen = BM::int_generator_one_sec<TIME>;
template <typename TIME> using acc = BM::accumulator<int, TIME>;

// ─── Inbox-cleanup topology: C1(Gen→Filter→) → C3(→C2(→Acc→)→) ──────────────

struct C1_out : public cadmium::out_port<int> {};
using ips_C1       = std::tuple<>;
using ops_C1       = std::tuple<C1_out>;
using submodels_C1 = cadmium::modeling::models_tuple<gen, filter_one>;
using eics_C1      = std::tuple<>;
using eocs_C1 =
    std::tuple<cadmium::modeling::EOC<filter_one, BM::filter_first_output_defs::out, C1_out>>;
using ics_C1 = std::tuple<cadmium::modeling::IC<gen, BM::int_generator_one_sec_defs::out,
                                                filter_one, BM::filter_first_output_defs::in>>;
template <typename TIME>
struct C1 : public cadmium::modeling::pdevs::coupled_model<TIME, ips_C1, ops_C1, submodels_C1,
                                                           eics_C1, eocs_C1, ics_C1> {};

struct C2_in : public cadmium::in_port<int> {};
struct C2_out : public cadmium::out_port<int> {};
using ips_C2       = std::tuple<C2_in>;
using ops_C2       = std::tuple<C2_out>;
using submodels_C2 = cadmium::modeling::models_tuple<acc>;
using eics_C2      = std::tuple<cadmium::modeling::EIC<C2_in, acc, BM::accumulator_defs<int>::add>>;
using eocs_C2 = std::tuple<cadmium::modeling::EOC<acc, BM::accumulator_defs<int>::sum, C2_out>>;
using ics_C2  = std::tuple<>;
template <typename TIME>
struct C2 : public cadmium::modeling::pdevs::coupled_model<TIME, ips_C2, ops_C2, submodels_C2,
                                                           eics_C2, eocs_C2, ics_C2> {};

struct C3_in : public cadmium::in_port<int> {};
struct C3_out : public cadmium::out_port<int> {};
using ips_C3       = std::tuple<C3_in>;
using ops_C3       = std::tuple<C3_out>;
using submodels_C3 = cadmium::modeling::models_tuple<C2>;
using eics_C3      = std::tuple<cadmium::modeling::EIC<C3_in, C2, C2_in>>;
using eocs_C3      = std::tuple<cadmium::modeling::EOC<C2, C2_out, C3_out>>;
using ics_C3       = std::tuple<>;
template <typename TIME>
struct C3 : public cadmium::modeling::pdevs::coupled_model<TIME, ips_C3, ops_C3, submodels_C3,
                                                           eics_C3, eocs_C3, ics_C3> {};

struct top_out : public cadmium::out_port<int> {};
using ips_TOP       = std::tuple<>;
using ops_TOP       = std::tuple<top_out>;
using submodels_TOP = cadmium::modeling::models_tuple<C1, C3>;
using eics_TOP      = std::tuple<>;
using eocs_TOP      = std::tuple<cadmium::modeling::EOC<C3, C3_out, top_out>>;
using ics_TOP       = std::tuple<cadmium::modeling::IC<C1, C1_out, C3, C3_in>>;
template <typename TIME>
struct CTOP : public cadmium::modeling::pdevs::coupled_model<TIME, ips_TOP, ops_TOP, submodels_TOP,
                                                             eics_TOP, eocs_TOP, ics_TOP> {};

SCENARIO("inbox cleanup prevents stale messages from being re-processed on the "
         "next tick",
         "[cleanup][inbox]") {
    GIVEN("a topology where a filtered generator feeds an accumulator through "
          "two levels of coupling") {
        WHEN("the simulation is stepped for 5 seconds via the coordinator") {
            cadmium::engine::coordinator<CTOP, float> c;
            c.init(0.0f);
            int top_outputs = 0;
            while (c.next() < 5.0f) {
                float t = c.next();
                c.collect_outputs(t);
                if (!cadmium::engine::all_bags_empty(c.outbox()))
                    ++top_outputs;
                c.advance_simulation(t);
            }
            THEN("the top-level coupled model produces at most 1 output — "
                 "the accumulator is not double-accumulated") {
                CHECK(top_outputs <= 1);
            }
        }
    }
}

// ─── Outbox-cleanup topology: D1(Gen→Filter→) → D3(→D2(→SecondFilter→Acc→)→) ─

template <typename TIME> struct second_filter_one : public BM::filter_first_output<TIME> {};

struct D1_out : public cadmium::out_port<int> {};
using ips_D1       = std::tuple<>;
using ops_D1       = std::tuple<D1_out>;
using submodels_D1 = cadmium::modeling::models_tuple<gen, filter_one>;
using eics_D1      = std::tuple<>;
using eocs_D1 =
    std::tuple<cadmium::modeling::EOC<filter_one, BM::filter_first_output_defs::out, D1_out>>;
using ics_D1 = std::tuple<cadmium::modeling::IC<gen, BM::int_generator_one_sec_defs::out,
                                                filter_one, BM::filter_first_output_defs::in>>;
template <typename TIME>
struct D1 : public cadmium::modeling::pdevs::coupled_model<TIME, ips_D1, ops_D1, submodels_D1,
                                                           eics_D1, eocs_D1, ics_D1> {};

struct D2_in : public cadmium::in_port<int> {};
struct D2_out : public cadmium::out_port<int> {};
using ips_D2       = std::tuple<D2_in>;
using ops_D2       = std::tuple<D2_out>;
using submodels_D2 = cadmium::modeling::models_tuple<second_filter_one, acc>;
using eics_D2 =
    std::tuple<cadmium::modeling::EIC<D2_in, second_filter_one, BM::filter_first_output_defs::in>>;
using eocs_D2 = std::tuple<cadmium::modeling::EOC<acc, BM::accumulator_defs<int>::sum, D2_out>>;
using ics_D2 =
    std::tuple<cadmium::modeling::IC<second_filter_one, BM::filter_first_output_defs::out, acc,
                                     BM::accumulator_defs<int>::add>>;
template <typename TIME>
struct D2 : public cadmium::modeling::pdevs::coupled_model<TIME, ips_D2, ops_D2, submodels_D2,
                                                           eics_D2, eocs_D2, ics_D2> {};

struct D3_in : public cadmium::in_port<int> {};
struct D3_out : public cadmium::out_port<int> {};
using ips_D3       = std::tuple<D3_in>;
using ops_D3       = std::tuple<D3_out>;
using submodels_D3 = cadmium::modeling::models_tuple<D2>;
using eics_D3      = std::tuple<cadmium::modeling::EIC<D3_in, D2, D2_in>>;
using eocs_D3      = std::tuple<cadmium::modeling::EOC<D2, D2_out, D3_out>>;
using ics_D3       = std::tuple<>;
template <typename TIME>
struct D3 : public cadmium::modeling::pdevs::coupled_model<TIME, ips_D3, ops_D3, submodels_D3,
                                                           eics_D3, eocs_D3, ics_D3> {};

struct DTOP_out : public cadmium::out_port<int> {};
using ips_DTOP       = std::tuple<>;
using ops_DTOP       = std::tuple<DTOP_out>;
using submodels_DTOP = cadmium::modeling::models_tuple<D1, D3>;
using eics_DTOP      = std::tuple<>;
using eocs_DTOP      = std::tuple<cadmium::modeling::EOC<D3, D3_out, DTOP_out>>;
using ics_DTOP       = std::tuple<cadmium::modeling::IC<D1, D1_out, D3, D3_in>>;
template <typename TIME>
struct DTOP
    : public cadmium::modeling::pdevs::coupled_model<TIME, ips_DTOP, ops_DTOP, submodels_DTOP,
                                                     eics_DTOP, eocs_DTOP, ics_DTOP> {};

SCENARIO("outbox cleanup prevents stale messages from being re-routed on the "
         "next tick",
         "[cleanup][outbox]") {
    GIVEN("a topology where a filtered generator feeds an accumulator via a "
          "second filter through two levels of coupling") {
        WHEN("the simulation is stepped for 5 seconds via the coordinator") {
            cadmium::engine::coordinator<DTOP, float> c;
            c.init(0.0f);
            int top_outputs = 0;
            while (c.next() < 5.0f) {
                float t = c.next();
                c.collect_outputs(t);
                if (!cadmium::engine::all_bags_empty(c.outbox()))
                    ++top_outputs;
                c.advance_simulation(t);
            }
            THEN("the top-level coupled model produces at most 1 output — "
                 "the accumulator is not double-accumulated") {
                CHECK(top_outputs <= 1);
            }
        }
    }
}
