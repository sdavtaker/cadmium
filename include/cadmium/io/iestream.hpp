// SPDX-License-Identifier: BSD-2-Clause
/**
 * Copyright (c) 2017-present, Cristina Ruiz Martin, Laouen Mayal Louan Belloli
 * Carleton University, Universidad de Valladolid
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

#ifndef CADMIUM_IESTREAM_HPP
#define CADMIUM_IESTREAM_HPP

#include <cadmium/modeling/message_bag.hpp>
#include <cadmium/modeling/ports.hpp>

#include <fstream>
#include <limits>
#include <stddef.h>
#include <stdexcept>
#include <string>

template <class TIME, class INPUT> class Parser {
  private:
    std::ifstream file;

  public:
    Parser() {}

    explicit Parser(const char *file_path) {
        open_file(file_path);
    }

    void open_file(const char *file_path) {
        file.open(file_path);
        if (!file.is_open())
            throw std::runtime_error(std::string("iestream: failed to open file: ") + file_path);
    }

    std::pair<TIME, INPUT> next_timed_input() {
        TIME next_time;
        INPUT result;
        if (!(file >> next_time))
            throw std::exception();
        if (!(file >> result))
            throw std::exception();
        return std::make_pair(next_time, result);
    }
};

template <typename MSG> struct iestream_input_defs {
    struct out : public cadmium::out_port<MSG> {};
};

template <typename MSG, typename TIME, typename PORT_TYPE> class iestream_input {
    using defs = PORT_TYPE;

  public:
    iestream_input() noexcept {}
    explicit iestream_input(const char *file_path) {
        state._parser.open_file(file_path);
    }

    struct state_type {
        Parser<TIME, MSG> _parser;
        MSG _last_input_read;
        std::vector<MSG> _next_input;
        TIME _simulation_time = TIME();
        TIME _next_time       = TIME();
        TIME _next_time2      = TIME();
        bool _initialization  = true;
    };

    state_type state;

    using input_ports  = std::tuple<>;
    using output_ports = std::tuple<typename defs::out>;

    void internal_transition() {
        state._simulation_time += state._next_time;
        state._next_input.clear();
        if (state._initialization) {
            state._initialization = false;
            try {
                std::pair<TIME, MSG> parsed_line = state._parser.next_timed_input();
                state._next_time                 = parsed_line.first - state._simulation_time;
                if (state._next_time < TIME({0}))
                    throw std::exception();
                state._last_input_read = parsed_line.second;
                state._next_input.push_back(state._last_input_read);
            } catch (std::exception &) {
                state._next_time = std::numeric_limits<TIME>::infinity();
            }
            try {
                std::pair<TIME, MSG> parsed_line = state._parser.next_timed_input();
                state._next_time2                = parsed_line.first - state._simulation_time;
                if (state._next_time2 < TIME({0}))
                    throw std::exception();
                state._last_input_read = parsed_line.second;
            } catch (std::exception &) {
                state._next_time2 = std::numeric_limits<TIME>::infinity();
            }
        } else {
            state._next_time = state._next_time2 - state._next_time;
            state._next_input.push_back(state._last_input_read);
            try {
                std::pair<TIME, MSG> parsed_line = state._parser.next_timed_input();
                state._next_time2                = parsed_line.first - state._simulation_time;
                if (state._next_time2 < TIME({0}))
                    throw std::exception();
                state._last_input_read = parsed_line.second;
            } catch (std::exception &) {
                state._next_time2 = std::numeric_limits<TIME>::infinity();
            }
        }
        while (state._next_time == state._next_time2 &&
               state._next_time != std::numeric_limits<TIME>::infinity()) {
            state._next_input.push_back(state._last_input_read);
            try {
                std::pair<TIME, MSG> parsed_line = state._parser.next_timed_input();
                state._next_time2                = parsed_line.first - state._simulation_time;
                if (state._next_time2 < TIME({0}))
                    throw std::exception();
                state._last_input_read = parsed_line.second;
            } catch (std::exception &) {
                state._next_time2 = std::numeric_limits<TIME>::infinity();
            }
        }
    }

    void external_transition(TIME, typename cadmium::make_message_bags<input_ports>::type) {
        throw std::logic_error("External transition called in a model with no input ports");
    }

    void confluence_transition(TIME, typename cadmium::make_message_bags<input_ports>::type) {
        throw std::logic_error("Confluence transition called in a model with no input ports");
    }

    typename cadmium::make_message_bags<output_ports>::type output() const {
        typename cadmium::make_message_bags<output_ports>::type bags;
        for (const auto &v : state._next_input)
            cadmium::get_messages<typename defs::out>(bags).emplace_back(v);
        return bags;
    }

    TIME time_advance() const {
        return state._next_time;
    }

    friend std::ostringstream &
    operator<<(std::ostringstream &os,
               const typename iestream_input<MSG, TIME, PORT_TYPE>::state_type &i) {
        os << "next time: " << i._next_time;
        return os;
    }
};

#endif // CADMIUM_IESTREAM_HPP
