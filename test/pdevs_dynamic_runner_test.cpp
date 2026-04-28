/**
 * Copyright (c) 2018-present, Laouen M. L. Belloli, Damian Vicino
 * Carleton University, Universite de Nice-Sophia Antipolis, Universidad de Buenos Aires
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

#include <catch2/catch_test_macros.hpp>
#include <sstream>
#include <string>

#include <cadmium/basic_model/pdevs/generator.hpp>
#include <cadmium/engine/pdevs_dynamic_runner.hpp>
#include <cadmium/modeling/coupling.hpp>
#include <cadmium/modeling/dynamic_atomic.hpp>
#include <cadmium/modeling/dynamic_coupled.hpp>
#include <cadmium/modeling/dynamic_model_translator.hpp>

namespace {
    struct test_tick {};

    using out_port = cadmium::basic_models::pdevs::generator_defs<test_tick>::out;

    template<typename TIME>
    using test_tick_generator_base = cadmium::basic_models::pdevs::generator<test_tick, TIME>;

    template<typename TIME>
    struct test_generator : public test_tick_generator_base<TIME> {
        float period() const override { return 1.0f; }
        test_tick output_message() const override { return test_tick{}; }
    };

    struct coupled_out_port : public cadmium::out_port<test_tick> {};

    using iports    = std::tuple<>;
    using oports    = std::tuple<coupled_out_port>;
    using submodels = cadmium::modeling::models_tuple<test_generator>;
    using eics      = std::tuple<>;
    using eocs      = std::tuple<cadmium::modeling::EOC<test_generator, out_port, coupled_out_port>>;
    using ics       = std::tuple<>;

    template<typename TIME>
    using coupled_generator = cadmium::modeling::pdevs::coupled_model<TIME, iports, oports, submodels, eics, eocs, ics>;

    auto coupled        = cadmium::dynamic::translate::make_dynamic_coupled_model<float, coupled_generator>();
    auto sp_test_generator = cadmium::dynamic::translate::make_dynamic_atomic_model<test_generator, float>();

    std::ostringstream oss;

    struct oss_test_sink_provider {
        static std::ostream& sink() { return oss; }
    };
}

TEST_CASE("dynamic runner of generator-in-coupled runs 60 seconds", "[dynamic][runner]") {
    cadmium::dynamic::engine::runner<float, cadmium::logger::not_logger> r(coupled, 0.0);
    float end = r.run_until(60.0);
    CHECK(end == 60.0f);
}

TEST_CASE("dynamic runner logs global time advances", "[dynamic][runner][logger]") {
    oss.str("");
    using log_gt = cadmium::logger::logger<
        cadmium::logger::logger_global_time,
        cadmium::dynamic::logger::formatter<float>,
        oss_test_sink_provider>;

    cadmium::dynamic::engine::runner<float, log_gt> r(coupled, 0.0);
    r.run_until(3.0);

    std::string expected = "0\n1\n2\n";
    CHECK(oss.str() == expected);
}

TEST_CASE("dynamic runner logs info on init and simulation loop", "[dynamic][runner][logger]") {
    oss.str("");
    using log_info = cadmium::logger::logger<
        cadmium::logger::logger_info,
        cadmium::dynamic::logger::formatter<float>,
        oss_test_sink_provider>;

    cadmium::dynamic::engine::runner<float, log_info> r(coupled, 0.0);
    r.run_until(2.0);

    std::ostringstream expected;
    expected << "Preparing model\n";
    expected << "Coordinator for model " << coupled->get_id() << " initialized to time 0\n";
    expected << "Simulator for model " << sp_test_generator->get_id() << " initialized to time 0\n";
    expected << "Starting run\n";
    expected << "Coordinator for model " << coupled->get_id() << " collecting output at time 1\n";
    expected << "Simulator for model " << sp_test_generator->get_id() << " collecting output at time 1\n";
    expected << "Coordinator for model " << coupled->get_id() << " advancing simulation from time 0 to 1\n";
    expected << "Simulator for model " << sp_test_generator->get_id() << " advancing simulation from time 0 to 1\n";
    expected << "Finished run\n";

    CHECK(oss.str() == expected.str());
}

TEST_CASE("dynamic runner logs state changes at init and each tick", "[dynamic][runner][logger]") {
    oss.str("");
    using log_state = cadmium::logger::logger<
        cadmium::logger::logger_state,
        cadmium::dynamic::logger::formatter<float>,
        oss_test_sink_provider>;

    cadmium::dynamic::engine::runner<float, log_state> r(coupled, 0.0);
    r.run_until(3.0);

    std::ostringstream expected;
    for (int i = 0; i < 3; ++i) {
        expected << "State for model " << sp_test_generator->get_id() << " is 0\n";
    }
    CHECK(oss.str() == expected.str());
}

TEST_CASE("dynamic runner logs local time elapsed in simulator", "[dynamic][runner][logger]") {
    oss.str("");
    using log_local = cadmium::logger::logger<
        cadmium::logger::logger_local_time,
        cadmium::dynamic::logger::formatter<float>,
        oss_test_sink_provider>;

    cadmium::dynamic::engine::runner<float, log_local> r(coupled, 0.0);
    r.run_until(2.0);

    std::string expected = "Elapsed in model " + sp_test_generator->get_id() + " is 1s\n";
    CHECK(oss.str() == expected);
}
