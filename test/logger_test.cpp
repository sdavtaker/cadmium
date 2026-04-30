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

#include <cadmium/logger/cadmium_log.hpp>
#include <spdlog/sinks/ostream_sink.h>

namespace {
std::ostringstream oss;
} // namespace

static void setup_test_logger() {
  oss.str("");
  auto sink = std::make_shared<spdlog::sinks::ostream_sink_mt>(oss);
  auto log = std::make_shared<spdlog::logger>("cadmium", sink);
  log->set_pattern("%v");
  log->set_level(spdlog::level::debug);
  cadmium::log::detail::instance() = log;
}

SCENARIO("emit writes a valid NDJSON line when the logger is initialised",
         "[logger][ndjson]") {
  GIVEN("a logger writing to an in-memory stream") {
    setup_test_logger();
    WHEN("an info event is emitted") {
      cadmium::log::emit(cadmium::log::level::info, "run_info", "test message");
      cadmium::log::flush();
      THEN("the output contains the event name and message") {
        CHECK(oss.str().find("run_info") != std::string::npos);
        CHECK(oss.str().find("test message") != std::string::npos);
      }
    }
  }
}

SCENARIO("emit with sim_time includes the sim_time field in the JSON",
         "[logger][ndjson]") {
  GIVEN("a logger writing to an in-memory stream") {
    setup_test_logger();
    WHEN("an event with sim_time 1.5 is emitted") {
      cadmium::log::emit(cadmium::log::level::debug, "sim_state", "s0", 1.5);
      cadmium::log::flush();
      THEN("the output contains sim_time") {
        CHECK(oss.str().find("sim_time") != std::string::npos);
        CHECK(oss.str().find("sim_state") != std::string::npos);
      }
    }
  }
}

SCENARIO("emit is a no-op when the logger is not initialised",
         "[logger][noop]") {
  GIVEN("no logger has been initialised") {
    cadmium::log::detail::instance() = nullptr;
    WHEN("an event is emitted") {
      THEN("no exception is thrown") {
        CHECK_NOTHROW(
            cadmium::log::emit(cadmium::log::level::info, "ev", "msg"));
      }
    }
  }
}
