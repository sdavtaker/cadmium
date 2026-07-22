/**
 * Copyright (c) 2026-present, Damian Vicino
 * Carleton University
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

#include <cadmium/logger/common_loggers_helpers.hpp>
#include <cadmium/modeling/named.hpp>

#include <catch2/catch_test_macros.hpp>
#include <string_view>

namespace {

    struct unnamed_model {};

    struct named_model : cadmium::named<"my_channel"> {};

    // A type satisfying Named directly, without inheriting from named<>.
    struct hand_rolled_named {
        static constexpr std::string_view model_name = "hand_rolled";
    };

} // namespace

SCENARIO("fixed_string stores a literal usable as a structural NTTP", "[named]") {
    GIVEN("two fixed_string values built from the same literal") {
        constexpr cadmium::fixed_string a("foo");
        constexpr cadmium::fixed_string b("foo");
        constexpr cadmium::fixed_string c("bar");
        THEN("equality compares the underlying characters") {
            STATIC_REQUIRE(a == b);
            STATIC_REQUIRE_FALSE(a == c);
        }
        THEN("view() excludes the trailing null terminator") {
            STATIC_REQUIRE(a.view() == std::string_view("foo"));
            STATIC_REQUIRE(a.view().size() == 3);
        }
    }
}

SCENARIO("named<Name> makes a type satisfy the Named concept", "[named]") {
    GIVEN("a type inheriting from named<\"my_channel\">") {
        THEN("it satisfies cadmium::Named") {
            STATIC_REQUIRE(cadmium::Named<named_model>);
        }
        THEN("its model_name matches the literal it was given") {
            STATIC_REQUIRE(std::string_view(named_model::model_name) == "my_channel");
        }
    }
}

SCENARIO("Named is a structural check, not tied to inheriting from named<>", "[named]") {
    GIVEN("a type that declares its own static constexpr model_name") {
        THEN("it still satisfies cadmium::Named") {
            STATIC_REQUIRE(cadmium::Named<hand_rolled_named>);
        }
    }
}

SCENARIO("an ordinary type with no model_name does not satisfy Named", "[named]") {
    GIVEN("a plain struct with no model_name member") {
        THEN("it does not satisfy cadmium::Named") {
            STATIC_REQUIRE_FALSE(cadmium::Named<unnamed_model>);
        }
    }
}

SCENARIO("model_type_name prefers the compile-time Named member", "[named][logger]") {
    GIVEN("a type inheriting from named<\"my_channel\">") {
        THEN("model_type_name returns the constexpr name, not a typeid-derived string") {
            CHECK(cadmium::logger::model_type_name<named_model>() == "my_channel");
        }
    }
}

SCENARIO("model_type_name still falls back to typeid for unnamed types", "[named][logger]") {
    GIVEN("a plain struct with no model_name member") {
        THEN("model_type_name returns typeid(T).name(), unchanged from before this feature") {
            CHECK(cadmium::logger::model_type_name<unnamed_model>() ==
                  typeid(unnamed_model).name());
        }
    }
}
