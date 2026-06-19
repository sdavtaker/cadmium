// SPDX-License-Identifier: BSD-2-Clause
#include <cadmium/basic_model/qss/qss_cross_detect.hpp>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <limits>

using namespace cadmium::basic_models::qss;
using Catch::Matchers::WithinAbs;

using CD = qss_cross_detect<double>;

static auto make_u(double u) {
    typename cadmium::make_message_box<CD::input_ports>::type mb;
    cadmium::get_message<CD::in_u>(mb).emplace(u);
    return mb;
}

static auto make_Du(double Du) {
    typename cadmium::make_message_box<CD::input_ports>::type mb;
    cadmium::get_message<CD::in_Du>(mb).emplace(Du);
    return mb;
}

static auto make_u_Du(double u, double Du) {
    typename cadmium::make_message_box<CD::input_ports>::type mb;
    cadmium::get_message<CD::in_u>(mb).emplace(u);
    cadmium::get_message<CD::in_Du>(mb).emplace(Du);
    return mb;
}

static double get_out(const typename cadmium::make_message_box<CD::output_ports>::type &box) {
    return cadmium::get_message<CD::out>(box).value();
}

TEST_CASE("qss_cross_detect: initial sigma is infinity", "[qss_cross_detect]") {
    CD cd(5.0);
    REQUIRE(cd.time_advance() == std::numeric_limits<double>::infinity());
}

TEST_CASE("qss_cross_detect: rising signal schedules correct sigma", "[qss_cross_detect]") {
    // u=0, Du=1, threshold=5 → sigma = (5-0)/1 = 5.0
    CD cd(5.0);
    cd.external_transition(0.0, make_u_Du(0.0, 1.0));
    REQUIRE_THAT(cd.time_advance(), WithinAbs(5.0, 1e-12));
}

TEST_CASE("qss_cross_detect: partial approach shortens sigma", "[qss_cross_detect]") {
    // u=3, Du=2, threshold=5 → sigma = (5-3)/2 = 1.0
    CD cd(5.0);
    cd.external_transition(0.0, make_u_Du(3.0, 2.0));
    REQUIRE_THAT(cd.time_advance(), WithinAbs(1.0, 1e-12));
}

TEST_CASE("qss_cross_detect: falling signal schedules correct sigma", "[qss_cross_detect]") {
    // u=10, Du=-2, threshold=5 → sigma = (5-10)/(-2) = 2.5
    CD cd(5.0);
    cd.external_transition(0.0, make_u_Du(10.0, -2.0));
    REQUIRE_THAT(cd.time_advance(), WithinAbs(2.5, 1e-12));
}

TEST_CASE("qss_cross_detect: zero derivative gives infinity", "[qss_cross_detect]") {
    CD cd(5.0);
    cd.external_transition(0.0, make_u_Du(0.0, 0.0));
    REQUIRE(cd.time_advance() == std::numeric_limits<double>::infinity());
}

TEST_CASE("qss_cross_detect: already past threshold gives sigma=0", "[qss_cross_detect]") {
    // u=6, Du=1, threshold=5 → t = (5-6)/1 = -1 ≤ 0 → sigma=0
    CD cd(5.0);
    cd.external_transition(0.0, make_u_Du(6.0, 1.0));
    REQUIRE(cd.time_advance() == 0.0);
}

TEST_CASE("qss_cross_detect: upward crossing outputs +1", "[qss_cross_detect]") {
    CD cd(5.0);
    cd.external_transition(0.0, make_u_Du(0.0, 1.0));
    cd.internal_transition();
    // After internal, sigma resets to infinity and next output should be +1
    // Output is checked just before internal fires (sigma was > 0, now triggered)
    // Re-inject to check output
    cd.external_transition(0.0, make_u_Du(4.9, 1.0));
    REQUIRE(cd.time_advance() > 0.0);
    auto box = cd.output();
    REQUIRE_THAT(get_out(box), WithinAbs(1.0, 1e-12));
}

TEST_CASE("qss_cross_detect: downward crossing outputs -1", "[qss_cross_detect]") {
    CD cd(5.0);
    cd.external_transition(0.0, make_u_Du(10.0, -2.0));
    auto box = cd.output();
    REQUIRE_THAT(get_out(box), WithinAbs(-1.0, 1e-12));
}

TEST_CASE("qss_cross_detect: internal transition resets to infinity", "[qss_cross_detect]") {
    CD cd(5.0);
    cd.external_transition(0.0, make_u_Du(0.0, 1.0));
    cd.internal_transition();
    REQUIRE(cd.time_advance() == std::numeric_limits<double>::infinity());
}

TEST_CASE("qss_cross_detect: port updates are independent", "[qss_cross_detect]") {
    // Send u and Du separately; second update should recompute sigma correctly
    CD cd(5.0);
    cd.external_transition(0.0, make_u(2.0));
    // Du still 0 → infinity
    REQUIRE(cd.time_advance() == std::numeric_limits<double>::infinity());

    cd.external_transition(0.0, make_Du(1.0));
    // Now u=2, Du=1, threshold=5 → sigma = 3.0
    REQUIRE_THAT(cd.time_advance(), WithinAbs(3.0, 1e-12));
}

TEST_CASE("qss_cross_detect: default threshold is 0", "[qss_cross_detect]") {
    // No arg → threshold=0; u=-3, Du=1 → sigma = (0-(-3))/1 = 3
    CD cd;
    cd.external_transition(0.0, make_u_Du(-3.0, 1.0));
    REQUIRE_THAT(cd.time_advance(), WithinAbs(3.0, 1e-12));
}
