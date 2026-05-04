// SPDX-License-Identifier: BSD-2-Clause
/**
 * Copyright (c) 2013-present, Damian Vicino, Laouen M. L. Belloli
 * Carleton University, Universite de Nice-Sophia Antipolis, Universidad de
 * Buenos Aires All rights reserved.
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

#pragma once

#include <chrono>
#include <exception>
#include <format>
#include <optional>
#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/spdlog.h>
#include <string>
#include <string_view>

namespace cadmium::log {

    enum class level { debug, info, warn, error };

    namespace detail {

        inline std::shared_ptr<spdlog::logger> &instance() noexcept {
            static std::shared_ptr<spdlog::logger> log;
            return log;
        }

        inline std::string_view level_str(level l) noexcept {
            switch (l) {
            case level::debug:
                return "debug";
            case level::info:
                return "info";
            case level::warn:
                return "warn";
            case level::error:
                return "error";
            }
            return "info";
        }

        inline std::string escape_json(std::string_view s) {
            std::string out;
            out.reserve(s.size());
            for (char c : s) {
                if (c == '"')
                    out += "\\\"";
                else if (c == '\\')
                    out += "\\\\";
                else if (c == '\n')
                    out += "\\n";
                else if (c == '\r')
                    out += "\\r";
                else if (c == '\t')
                    out += "\\t";
                else
                    out += c;
            }
            return out;
        }

    } // namespace detail

    // Converts a simulation TIME value to double for the optional sim_time JSON
    // field. Specialize for types where static_cast<double> is insufficient.
    template <typename T> double to_sim_double(const T &t) noexcept {
        return static_cast<double>(t);
    }

    inline void init() {
        auto sink = std::make_shared<spdlog::sinks::stdout_sink_mt>();
        auto log  = std::make_shared<spdlog::logger>("cadmium", sink);
        log->set_pattern("%v");
        log->set_level(spdlog::level::debug);
        spdlog::flush_on(spdlog::level::err);
        detail::instance() = log;
    }

    inline void emit(level lvl, std::string_view event, std::string_view msg,
                     std::optional<double> sim_time = std::nullopt) {
        auto &log = detail::instance();
        if (!log)
            return;

        auto now = std::chrono::system_clock::now();
        auto ts  = std::format("{0:%Y-%m-%dT%H:%M:%S}Z",
                               std::chrono::floor<std::chrono::milliseconds>(now));

        std::string line = std::format(R"({{"ts":"{}","level":"{}","event":"{}","msg":"{}"}})", ts,
                                       detail::level_str(lvl), detail::escape_json(event),
                                       detail::escape_json(msg));

        if (sim_time.has_value()) {
            line.pop_back();
            line += std::format(R"(,"sim_time":{:.17g}}})", *sim_time);
        }

        log->info(line);
    }

    inline void flush() noexcept {
        auto &log = detail::instance();
        if (log)
            log->flush();
    }

    inline void log_exception(const std::exception &e, std::string_view event) {
        emit(level::error, event, e.what());
        flush();
    }

} // namespace cadmium::log
