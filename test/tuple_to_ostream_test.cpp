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
#include <tuple>

#include <cadmium/logger/tuple_to_ostream.hpp>

using namespace cadmium;

SCENARIO("an empty tuple is streamed to an ostream", "[tuple_to_ostream]") {
  GIVEN("an empty tuple") {
    std::tuple<> empty;
    WHEN("it is streamed to an ostringstream") {
      std::ostringstream oss;
      oss << empty;
      THEN("the output is []") { CHECK(oss.str() == "[]"); }
    }
  }
}

SCENARIO("a single-element tuple is streamed to an ostream",
         "[tuple_to_ostream]") {
  GIVEN("a tuple containing the integer 1") {
    auto one_int = std::make_tuple(1);
    WHEN("it is streamed to an ostringstream") {
      std::ostringstream oss;
      oss << one_int;
      THEN("the output is [1]") { CHECK(oss.str() == "[1]"); }
    }
  }
}
