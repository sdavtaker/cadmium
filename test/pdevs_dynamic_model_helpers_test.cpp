/**
 * Copyright (c) 2018-present, Laouen M. Louan Belloli
 * Carleton University, Universidad de Buenos Aires
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
#include <any>

#include <cadmium/modeling/dynamic_message_bag.hpp>
#include <cadmium/modeling/dynamic_model_translator.hpp>
#include <cadmium/modeling/dynamic_models_helpers.hpp>
#include <cadmium/modeling/message_bag.hpp>
#include <cadmium/modeling/ports.hpp>

// Port types must be at namespace scope — local structs cannot be template arguments in C++
namespace {
    struct mh_test_out : public cadmium::out_port<int> {};
    struct mh_test_in  : public cadmium::in_port<int>  {};

    struct mh_in_port_0 : public cadmium::in_port<int>  {};
    struct mh_in_port_1 : public cadmium::in_port<int>  {};
    struct mh_out_port_0 : public cadmium::out_port<int> {};
}

TEST_CASE("link creation records correct from/to port type indices", "[dynamic][link]") {
    auto link = cadmium::dynamic::translate::make_link<mh_test_out, mh_test_in>();
    CHECK(link->from_port_type_index() == typeid(mh_test_out));
    CHECK(link->to_port_type_index()   == typeid(mh_test_in));
}

TEST_CASE("route_messages copies message to destination without modifying source", "[dynamic][link]") {
    auto link = cadmium::dynamic::translate::make_link<mh_test_out, mh_test_in>();

    cadmium::message_bag<mh_test_out> bag_out;
    bag_out.messages.push_back(3);

    cadmium::dynamic::message_bags bag_from;
    bag_from[link->from_port_type_index()] = bag_out;

    cadmium::dynamic::message_bags bag_to;
    link->route_messages(bag_from, bag_to);

    CHECK(std::any_cast<cadmium::message_bag<mh_test_out>>(
        bag_from.at(link->from_port_type_index())).messages.size() == 1);
    CHECK(std::any_cast<cadmium::message_bag<mh_test_in>>(
        bag_to.at(link->to_port_type_index())).messages.size() == 1);
    CHECK(std::any_cast<cadmium::message_bag<mh_test_in>>(
        bag_to.at(link->to_port_type_index())).messages.front() == 3);
}

TEST_CASE("route_messages called twice accumulates messages in destination", "[dynamic][link]") {
    auto link = cadmium::dynamic::translate::make_link<mh_test_out, mh_test_in>();

    cadmium::message_bag<mh_test_out> bag_out;
    bag_out.messages.push_back(3);
    cadmium::dynamic::message_bags bag_from;
    bag_from[link->from_port_type_index()] = bag_out;
    cadmium::dynamic::message_bags bag_to;

    link->route_messages(bag_from, bag_to);
    link->route_messages(bag_from, bag_to);

    CHECK(std::any_cast<cadmium::message_bag<mh_test_in>>(
        bag_to.at(link->to_port_type_index())).messages.size() == 2);
}

TEST_CASE("make_ports produces correct port type indices from tuple", "[dynamic][translate]") {
    using iports = std::tuple<mh_in_port_0, mh_in_port_1>;
    using oports = std::tuple<mh_out_port_0>;

    auto input_ports  = cadmium::dynamic::translate::make_ports<iports>();
    auto output_ports = cadmium::dynamic::translate::make_ports<oports>();

    REQUIRE(input_ports.size()  == 2);
    REQUIRE(output_ports.size() == 1);
    CHECK(input_ports[0]  == typeid(mh_in_port_0));
    CHECK(input_ports[1]  == typeid(mh_in_port_1));
    CHECK(output_ports[0] == typeid(mh_out_port_0));
}
