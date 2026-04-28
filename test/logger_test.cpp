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

#include <catch2/catch_test_macros.hpp>
#include <sstream>

#include <cadmium/logger/common_loggers.hpp>
#include <cadmium/logger/logger.hpp>

namespace {
    std::ostringstream oss;
    std::ostringstream oss2;

    struct sink1 { static std::ostream& sink() { return oss; } };
    struct sink2 { static std::ostream& sink() { return oss2; } };
}

TEST_CASE("logger ignores messages below its level", "[logger]") {
    oss.str("");
    cadmium::logger::logger<cadmium::logger::logger_info,
                            cadmium::logger::formatter<float>, sink1> l;
    l.log<cadmium::logger::logger_debug, cadmium::logger::run_info>("nothing to show");
    CHECK(oss.str().empty());
}

TEST_CASE("logger emits message at matching level", "[logger]") {
    oss.str("");
    cadmium::logger::logger<cadmium::logger::logger_info,
                            cadmium::logger::formatter<float>, sink1> l;
    l.log<cadmium::logger::logger_info, cadmium::logger::run_info>("something to show");
    CHECK(oss.str() == "something to show\n");
}

TEST_CASE("multilogger routes to each sink by level", "[logger][multilogger]") {
    oss.str("");
    oss2.str("");
    using log1 = cadmium::logger::logger<cadmium::logger::logger_info,
                                         cadmium::logger::formatter<float>, sink1>;
    using log2 = cadmium::logger::logger<cadmium::logger::logger_debug,
                                         cadmium::logger::formatter<float>, sink2>;
    cadmium::logger::multilogger<log1, log2> l;
    l.log<cadmium::logger::logger_info,  cadmium::logger::run_info>("some info");
    l.log<cadmium::logger::logger_debug, cadmium::logger::run_info>("some debug");
    CHECK(oss.str()  == "some info\n");
    CHECK(oss2.str() == "some debug\n");
}
