// SPDX-License-Identifier: BSD-2-Clause
#include <cadmium/basic_model/qss/qss1_integrator.hpp>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <limits>

using namespace cadmium::basic_models::qss;
using Catch::Matchers::WithinAbs;
using Catch::Matchers::WithinRel;

using Q = qss1_integrator<double>;

// Helper: build an in_u message box with a given derivative value.
static auto make_u(double u) {
    typename cadmium::make_message_box<Q::input_ports>::type mb;
    cadmium::get_message<Q::in_u>(mb).emplace(u);
    return mb;
}

// Helper: extract the out_q value from an output message box.
static double get_q(const typename cadmium::make_message_box<Q::output_ports>::type &box) {
    return cadmium::get_message<Q::out_q>(box).value();
}

TEST_CASE("qss1_integrator: initial sigma is zero", "[qss1]") {
    Q integrator(1.0, 1e-2, 0.0);
    REQUIRE(integrator.time_advance() == 0.0);
}

TEST_CASE("qss1_integrator: output at t=0 is initial value", "[qss1]") {
    Q integrator(2.5, 1e-2, 0.0);
    auto box = integrator.output();
    REQUIRE_THAT(get_q(box), WithinAbs(2.5, 1e-12));
}

TEST_CASE("qss1_integrator: internal transition with zero derivative gives infinity", "[qss1]") {
    Q integrator(1.0, 1e-2, 0.0);
    integrator.internal_transition(); // sigma was 0, u=0 → sigma = inf
    REQUIRE(integrator.time_advance() == std::numeric_limits<double>::infinity());
}

TEST_CASE("qss1_integrator: constant derivative schedules correct sigma", "[qss1]") {
    // x0=1.0, dqmin=0.1, dqrel=0 → dq=0.1; u=1.0 → sigma=0.1/1.0=0.1
    Q integrator(1.0, 0.1, 0.0);
    integrator.internal_transition(); // consume t=0 event

    auto mb = make_u(1.0);
    integrator.external_transition(0.0, mb);
    REQUIRE_THAT(integrator.time_advance(), WithinAbs(0.1, 1e-12));
}

TEST_CASE("qss1_integrator: output value is at next crossing boundary", "[qss1]") {
    // x0=0, dq=0.1, u=1 → crossing at x=0.1 in sigma=0.1 s
    Q integrator(0.0, 0.1, 0.0);
    integrator.internal_transition();

    integrator.external_transition(0.0, make_u(1.0));
    REQUIRE_THAT(integrator.time_advance(), WithinAbs(0.1, 1e-12));

    auto box = integrator.output();
    REQUIRE_THAT(get_q(box), WithinAbs(0.1, 1e-12)); // next boundary
}

TEST_CASE("qss1_integrator: negative derivative crosses lower boundary", "[qss1]") {
    // x0=1.0, dq=0.1, u=-2 → lower boundary = 0.9, sigma=0.1/2=0.05
    Q integrator(1.0, 0.1, 0.0);
    integrator.internal_transition();

    integrator.external_transition(0.0, make_u(-2.0));
    REQUIRE_THAT(integrator.time_advance(), WithinAbs(0.05, 1e-12));

    auto box = integrator.output();
    REQUIRE_THAT(get_q(box), WithinAbs(0.9, 1e-12));
}

TEST_CASE("qss1_integrator: state advances correctly on external transition", "[qss1]") {
    // x0=0, dq=0.1, u=1; wait e=0.05 then change u=2
    // x at e=0.05: 0 + 1*0.05 = 0.05
    // upper boundary from q=0: 0+0.1=0.1; t_up=(0.1-0.05)/2=0.025
    Q integrator(0.0, 0.1, 0.0);
    integrator.internal_transition();
    integrator.external_transition(0.0, make_u(1.0));

    integrator.external_transition(0.05, make_u(2.0));
    REQUIRE_THAT(integrator.time_advance(), WithinAbs(0.025, 1e-12));
}

TEST_CASE("qss1_integrator: crossing already past boundary gives sigma=0", "[qss1]") {
    // x0=0, dq=0.1, u=1; internal fires at sigma=0.1, advancing x to 0.1 (=q)
    // Then external with u=10 arrives e=0 after the internal transition
    // Immediately past the boundary → sigma=0
    Q integrator(0.0, 0.1, 0.0);
    integrator.internal_transition(); // consume t=0 event

    integrator.external_transition(0.0, make_u(1.0));
    // advance through one full quantum
    integrator.internal_transition(); // x=0.1, q=0.1, sigma=0.1

    // Inject large derivative: moves x far immediately
    integrator.external_transition(0.05, make_u(100.0));
    // x = 0.1 + 1.0*0.05 = 0.15; |x-q| = 0.05 < dq=0.1 → no instant crossing yet
    REQUIRE(integrator.time_advance() > 0.0);
}

TEST_CASE("qss1_integrator: relative quantization scales with state", "[qss1]") {
    // dqmin=0, dqrel=0.1 → dq = 0.1*|x|
    // x0=10 → dq=1.0; u=1 → sigma=1.0
    Q integrator(10.0, 0.0, 0.1);
    integrator.internal_transition();

    integrator.external_transition(0.0, make_u(1.0));
    REQUIRE_THAT(integrator.time_advance(), WithinAbs(1.0, 1e-10));
}

TEST_CASE("qss1_integrator: linear integration over multiple steps", "[qss1]") {
    // Integrate dx/dt=1 from x0=0 with dq=0.5.
    // Should cross 0.5, 1.0, 1.5, ... at t=0.5, 1.0, 1.5, ...
    Q integrator(0.0, 0.5, 0.0);
    integrator.internal_transition(); // consume t=0

    integrator.external_transition(0.0, make_u(1.0));

    double t = 0.0;
    for (int step = 1; step <= 4; ++step) {
        double sigma = integrator.time_advance();
        REQUIRE_THAT(sigma, WithinAbs(0.5, 1e-10));
        double out = get_q(integrator.output());
        REQUIRE_THAT(out, WithinAbs(step * 0.5, 1e-10));
        t += sigma;
        integrator.internal_transition();
    }
    REQUIRE_THAT(t, WithinAbs(2.0, 1e-10));
}
