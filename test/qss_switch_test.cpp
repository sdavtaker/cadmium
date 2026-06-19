// SPDX-License-Identifier: BSD-2-Clause
#include <cadmium/basic_model/qss/qss_switch.hpp>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <limits>

using namespace cadmium::basic_models::qss;
using Catch::Matchers::WithinAbs;

using SW = qss_switch<double>;

static auto make_in0(double v) {
    typename cadmium::make_message_box<SW::input_ports>::type mb;
    cadmium::get_message<SW::in0>(mb).emplace(v);
    return mb;
}

static auto make_in1(double v) {
    typename cadmium::make_message_box<SW::input_ports>::type mb;
    cadmium::get_message<SW::in1>(mb).emplace(v);
    return mb;
}

static auto make_sw(double sw) {
    typename cadmium::make_message_box<SW::input_ports>::type mb;
    cadmium::get_message<SW::in_sw>(mb).emplace(sw);
    return mb;
}

static double get_out(const typename cadmium::make_message_box<SW::output_ports>::type &box) {
    return cadmium::get_message<SW::out>(box).value();
}

TEST_CASE("qss_switch: initial sigma is infinity", "[qss_switch]") {
    SW sw;
    REQUIRE(sw.time_advance() == std::numeric_limits<double>::infinity());
}

TEST_CASE("qss_switch: any input triggers immediate output", "[qss_switch]") {
    SW sw;
    sw.external_transition(0.0, make_in0(3.0));
    REQUIRE(sw.time_advance() == 0.0);
}

TEST_CASE("qss_switch: sw=0 routes in0", "[qss_switch]") {
    SW sw;
    // Set both values
    {
        typename cadmium::make_message_box<SW::input_ports>::type mb;
        cadmium::get_message<SW::in0>(mb).emplace(3.0);
        cadmium::get_message<SW::in1>(mb).emplace(7.0);
        cadmium::get_message<SW::in_sw>(mb).emplace(0.0);
        sw.external_transition(0.0, mb);
    }
    REQUIRE_THAT(get_out(sw.output()), WithinAbs(3.0, 1e-12));
}

TEST_CASE("qss_switch: sw nonzero routes in1", "[qss_switch]") {
    SW sw;
    {
        typename cadmium::make_message_box<SW::input_ports>::type mb;
        cadmium::get_message<SW::in0>(mb).emplace(3.0);
        cadmium::get_message<SW::in1>(mb).emplace(7.0);
        cadmium::get_message<SW::in_sw>(mb).emplace(1.0);
        sw.external_transition(0.0, mb);
    }
    REQUIRE_THAT(get_out(sw.output()), WithinAbs(7.0, 1e-12));
}

TEST_CASE("qss_switch: switch flips output on sw change", "[qss_switch]") {
    SW sw;
    // Prime with both signals
    {
        typename cadmium::make_message_box<SW::input_ports>::type mb;
        cadmium::get_message<SW::in0>(mb).emplace(3.0);
        cadmium::get_message<SW::in1>(mb).emplace(7.0);
        cadmium::get_message<SW::in_sw>(mb).emplace(0.0);
        sw.external_transition(0.0, mb);
    }
    REQUIRE_THAT(get_out(sw.output()), WithinAbs(3.0, 1e-12));
    sw.internal_transition();

    // Now flip sw
    sw.external_transition(1.0, make_sw(1.0));
    REQUIRE(sw.time_advance() == 0.0);
    REQUIRE_THAT(get_out(sw.output()), WithinAbs(7.0, 1e-12));
}

TEST_CASE("qss_switch: internal transition resets to infinity", "[qss_switch]") {
    SW sw;
    sw.external_transition(0.0, make_in0(1.0));
    REQUIRE(sw.time_advance() == 0.0);
    sw.internal_transition();
    REQUIRE(sw.time_advance() == std::numeric_limits<double>::infinity());
}

TEST_CASE("qss_switch: negative sw value selects in1", "[qss_switch]") {
    SW sw;
    {
        typename cadmium::make_message_box<SW::input_ports>::type mb;
        cadmium::get_message<SW::in0>(mb).emplace(3.0);
        cadmium::get_message<SW::in1>(mb).emplace(7.0);
        cadmium::get_message<SW::in_sw>(mb).emplace(-1.0);
        sw.external_transition(0.0, mb);
    }
    REQUIRE_THAT(get_out(sw.output()), WithinAbs(7.0, 1e-12));
}

TEST_CASE("qss_switch: updates to in0 propagate when sw=0", "[qss_switch]") {
    SW sw;
    sw.external_transition(0.0, make_in0(3.0));
    sw.internal_transition();

    sw.external_transition(0.5, make_in0(9.0));
    REQUIRE(sw.time_advance() == 0.0);
    REQUIRE_THAT(get_out(sw.output()), WithinAbs(9.0, 1e-12));
}

TEST_CASE("qss_switch: updates to in1 propagate when sw=nonzero", "[qss_switch]") {
    SW sw;
    // Set sw to 1
    sw.external_transition(0.0, make_sw(1.0));
    sw.internal_transition();

    sw.external_transition(0.5, make_in1(42.0));
    REQUIRE(sw.time_advance() == 0.0);
    REQUIRE_THAT(get_out(sw.output()), WithinAbs(42.0, 1e-12));
}
