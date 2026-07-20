// SPDX-License-Identifier: BSD-2-Clause
/**
 * Pack-of-models coupling tests: a coupled model declaring a pack<M, N>
 * submodel must behave identically to the same model written with N
 * individual submodel entries, and the (pack, index) coupling entries must
 * route EIC/IC/EOC messages to/from the addressed element.
 */
#include <cadmium/engine/devs_coordinator.hpp>
#include <cadmium/engine/devs_engine_helpers.hpp>
#include <cadmium/modeling/coupling.hpp>
#include <cadmium/modeling/message_box.hpp>
#include <cadmium/modeling/pack.hpp>
#include <cadmium/modeling/ports.hpp>

#include <catch2/catch_test_macros.hpp>
#include <limits>
#include <optional>
#include <stdexcept>
#include <tuple>
#include <vector>

namespace {

    using cadmium::modeling::not_packed;

    // ── Test atomics ─────────────────────────────────────────────────────────

    struct gen_defs {
        struct out : public cadmium::out_port<int> {};
    };

    /// Emits 1 on `out` every second, forever.
    template <typename TIME> class one_sec_gen {
      public:
        using state_type = int; // emission count (for the state log)
        state_type state = 0;

        using input_ports  = std::tuple<>;
        using output_ports = std::tuple<gen_defs::out>;

        void internal_transition() {
            ++state;
        }
        void external_transition(TIME, typename cadmium::make_message_box<input_ports>::type) {
            throw std::logic_error("one_sec_gen has no inputs");
        }
        typename cadmium::make_message_box<output_ports>::type output() const {
            typename cadmium::make_message_box<output_ports>::type box;
            cadmium::get_message<gen_defs::out>(box).emplace(1);
            return box;
        }
        TIME time_advance() const {
            return TIME{1};
        }
    };

    struct tally_defs {
        struct in : public cadmium::in_port<int> {};
        struct out : public cadmium::out_port<int> {};
    };

    /// Accumulates received ints; after every receipt immediately emits the
    /// running total once.
    template <typename TIME> class tally {
      public:
        struct state_type {
            int total    = 0;
            bool pending = false;

            friend std::ostream &operator<<(std::ostream &os, const state_type &s) {
                return os << "total:" << s.total << (s.pending ? " pending" : "");
            }
        };
        state_type state{};

        using input_ports  = std::tuple<tally_defs::in>;
        using output_ports = std::tuple<tally_defs::out>;

        void internal_transition() {
            state.pending = false;
        }
        void external_transition(TIME, typename cadmium::make_message_box<input_ports>::type box) {
            const auto &msg = cadmium::get_message<tally_defs::in>(box);
            if (msg.has_value()) {
                state.total += *msg;
                state.pending = true;
            }
        }
        typename cadmium::make_message_box<output_ports>::type output() const {
            typename cadmium::make_message_box<output_ports>::type box;
            if (state.pending) {
                cadmium::get_message<tally_defs::out>(box).emplace(state.total);
            }
            return box;
        }
        TIME time_advance() const {
            return state.pending ? TIME{0} : std::numeric_limits<TIME>::infinity();
        }
    };

    // ── Shared top-model scaffolding ─────────────────────────────────────────

    struct top_total : public cadmium::out_port<int> {};
    struct top_a : public cadmium::out_port<int> {};
    struct top_b : public cadmium::out_port<int> {};
    struct top_c : public cadmium::out_port<int> {};
    struct top_d : public cadmium::out_port<int> {};

    using empty_iports = std::tuple<>;
    using empty_eic    = std::tuple<>;
    using top_oports   = std::tuple<top_total, top_a, top_b, top_c, top_d>;

    namespace m = cadmium::modeling;

    // Pack edition: one pack of four generators.
    template <typename TIME> using gens4 = m::pack<one_sec_gen, 4>::model<TIME>;

    using pack_submodels = m::models_tuple<tally, gens4>;
    using pack_ics =
        std::tuple<m::pack_IC<gens4, 0, gen_defs::out, tally, not_packed, tally_defs::in>,
                   m::pack_IC<gens4, 1, gen_defs::out, tally, not_packed, tally_defs::in>,
                   m::pack_IC<gens4, 2, gen_defs::out, tally, not_packed, tally_defs::in>,
                   m::pack_IC<gens4, 3, gen_defs::out, tally, not_packed, tally_defs::in>>;
    using pack_eocs = std::tuple<
        m::EOC<tally, tally_defs::out, top_total>, m::pack_EOC<gens4, 0, gen_defs::out, top_a>,
        m::pack_EOC<gens4, 1, gen_defs::out, top_b>, m::pack_EOC<gens4, 2, gen_defs::out, top_c>,
        m::pack_EOC<gens4, 3, gen_defs::out, top_d>>;

    template <typename TIME>
    using pack_top = m::devs::coupling<TIME, empty_iports, top_oports, pack_submodels, empty_eic,
                                       pack_eocs, pack_ics, cadmium::engine::devs::first_imminent>;

    // Plain edition: four individually-declared generator wrappers.
    template <typename TIME> class gen0 : public one_sec_gen<TIME> {};
    template <typename TIME> class gen1 : public one_sec_gen<TIME> {};
    template <typename TIME> class gen2 : public one_sec_gen<TIME> {};
    template <typename TIME> class gen3 : public one_sec_gen<TIME> {};

    using plain_submodels = m::models_tuple<tally, gen0, gen1, gen2, gen3>;
    using plain_ics       = std::tuple<m::IC<gen0, gen_defs::out, tally, tally_defs::in>,
                                       m::IC<gen1, gen_defs::out, tally, tally_defs::in>,
                                       m::IC<gen2, gen_defs::out, tally, tally_defs::in>,
                                       m::IC<gen3, gen_defs::out, tally, tally_defs::in>>;
    using plain_eocs =
        std::tuple<m::EOC<tally, tally_defs::out, top_total>, m::EOC<gen0, gen_defs::out, top_a>,
                   m::EOC<gen1, gen_defs::out, top_b>, m::EOC<gen2, gen_defs::out, top_c>,
                   m::EOC<gen3, gen_defs::out, top_d>>;

    template <typename TIME>
    using plain_top =
        m::devs::coupling<TIME, empty_iports, top_oports, plain_submodels, empty_eic, plain_eocs,
                          plain_ics, cadmium::engine::devs::first_imminent>;

    struct observation {
        double t;
        std::optional<int> total, a, b, c, d;

        friend bool operator==(const observation &, const observation &) = default;
    };

    template <template <typename> class TOP> std::vector<observation> run_top(double t_end) {
        cadmium::engine::devs::coordinator<TOP, double> coord;
        coord.init(0.0);
        std::vector<observation> trace;
        auto next = coord.next();
        while (next < t_end) {
            coord.collect_outputs(next);
            trace.push_back(
                {next, coord.template outbox_port<top_total>(), coord.template outbox_port<top_a>(),
                 coord.template outbox_port<top_b>(), coord.template outbox_port<top_c>(),
                 coord.template outbox_port<top_d>()});
            coord.advance_simulation(next);
            next = coord.next();
        }
        return trace;
    }

} // namespace

TEST_CASE("pack of N generators behaves identically to N individual generators") {
    const auto pack_trace  = run_top<pack_top>(5.5);
    const auto plain_trace = run_top<plain_top>(5.5);

    REQUIRE_FALSE(pack_trace.empty());
    REQUIRE(pack_trace.size() == plain_trace.size());
    for (std::size_t i = 0; i < pack_trace.size(); ++i) {
        CAPTURE(i, pack_trace[i].t, plain_trace[i].t);
        CHECK(pack_trace[i] == plain_trace[i]);
    }
}

TEST_CASE("within-pack SELECT ties break by ascending element index") {
    const auto trace = run_top<pack_top>(1.5);
    // At t=1 the four elements are simultaneously imminent; per-element EOC
    // ports must fire in a, b, c, d order across successive events.
    std::vector<int> order;
    for (const auto &ob : trace) {
        if (ob.a.has_value()) {
            order.push_back(0);
        }
        if (ob.b.has_value()) {
            order.push_back(1);
        }
        if (ob.c.has_value()) {
            order.push_back(2);
        }
        if (ob.d.has_value()) {
            order.push_back(3);
        }
    }
    REQUIRE(order.size() == 4);
    CHECK(order == std::vector<int>{0, 1, 2, 3});
}

// ── pack_EIC: external input into a chosen element ──────────────────────────

namespace {

    struct top_in : public cadmium::in_port<int> {};
    struct top_t0 : public cadmium::out_port<int> {};
    struct top_t1 : public cadmium::out_port<int> {};

    template <typename TIME> using tallies2 = m::pack<tally, 2>::model<TIME>;

    using eic_submodels = m::models_tuple<tallies2>;
    using eic_iports    = std::tuple<top_in>;
    using eic_oports    = std::tuple<top_t0, top_t1>;
    using eic_eics      = std::tuple<m::pack_EIC<top_in, tallies2, 1, tally_defs::in>>;
    using eic_eocs      = std::tuple<m::pack_EOC<tallies2, 0, tally_defs::out, top_t0>,
                                     m::pack_EOC<tallies2, 1, tally_defs::out, top_t1>>;

    template <typename TIME>
    using eic_top =
        m::devs::coupling<TIME, eic_iports, eic_oports, eic_submodels, eic_eics, eic_eocs,
                          std::tuple<>, cadmium::engine::devs::first_imminent>;

} // namespace

TEST_CASE("pack_EIC routes coupled-model input to the addressed element only") {
    cadmium::engine::devs::coordinator<eic_top, double> coord;
    coord.init(0.0);

    coord.template set_inbox_port<top_in>(std::optional<int>{7});
    coord.advance_simulation(0.5); // deliver external input to element 1

    // Element 1 becomes imminent (ta = 0) and emits its total on top_t1.
    REQUIRE(coord.next() == 0.5);
    coord.collect_outputs(0.5);
    CHECK_FALSE(coord.template outbox_port<top_t0>().has_value());
    REQUIRE(coord.template outbox_port<top_t1>().has_value());
    CHECK(*coord.template outbox_port<top_t1>() == 7);
    coord.advance_simulation(0.5);
    CHECK(coord.next() == std::numeric_limits<double>::infinity());
}

// ── pack_IC: plain→pack and same-pack element→element routing ───────────────

namespace {

    struct top_u0 : public cadmium::out_port<int> {};
    struct top_u1 : public cadmium::out_port<int> {};

    template <typename TIME> using chain2 = m::pack<tally, 2>::model<TIME>;

    using chain_submodels = m::models_tuple<one_sec_gen, chain2>;
    using chain_oports    = std::tuple<top_u0, top_u1>;
    // gen (plain) feeds element 0; element 0's emissions feed element 1.
    using chain_ics =
        std::tuple<m::pack_IC<one_sec_gen, not_packed, gen_defs::out, chain2, 0, tally_defs::in>,
                   m::pack_IC<chain2, 0, tally_defs::out, chain2, 1, tally_defs::in>>;
    using chain_eocs = std::tuple<m::pack_EOC<chain2, 0, tally_defs::out, top_u0>,
                                  m::pack_EOC<chain2, 1, tally_defs::out, top_u1>>;

    template <typename TIME>
    using chain_top =
        m::devs::coupling<TIME, empty_iports, chain_oports, chain_submodels, empty_eic, chain_eocs,
                          chain_ics, cadmium::engine::devs::first_imminent>;

} // namespace

TEST_CASE("pack_IC routes plain->pack and same-pack element-to-element") {
    cadmium::engine::devs::coordinator<chain_top, double> coord;
    coord.init(0.0);

    std::vector<int> u0_seq, u1_seq;
    auto next = coord.next();
    while (next < 2.5) {
        coord.collect_outputs(next);
        if (const auto &v = coord.template outbox_port<top_u0>(); v.has_value()) {
            u0_seq.push_back(*v);
        }
        if (const auto &v = coord.template outbox_port<top_u1>(); v.has_value()) {
            u1_seq.push_back(*v);
        }
        coord.advance_simulation(next);
        next = coord.next();
    }

    // Each second the generator adds 1 to element 0, whose emitted running
    // total (1, then 2) is forwarded into element 1 — which accumulates
    // those totals: 1, then 1 + 2 = 3.
    CHECK(u0_seq == std::vector<int>{1, 2});
    CHECK(u1_seq == std::vector<int>{1, 3});
}
