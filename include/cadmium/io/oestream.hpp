// SPDX-License-Identifier: BSD-2-Clause
/**
 * Copyright (c) 2019-present, Ben Earle
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

#ifndef CADMIUM_OESTREAM_HPP
#define CADMIUM_OESTREAM_HPP

#include <cadmium/modeling/message_bag.hpp>
#include <cadmium/modeling/ports.hpp>

#include <fstream>
#include <limits>
#include <stddef.h>
#include <stdexcept>
#include <string>

/**
 * @brief output_event_stream PDEVS Model.
 *
 * oestream_output is a simple model that prints the values passed to its
 * input port to a file.
 *
 */

template <typename MSG, typename TIME, typename PORT_TYPE> class oestream_output {
    using defs = PORT_TYPE;

  public:
    oestream_output() noexcept {}
    explicit oestream_output(const char *file_path) {
        state.path = file_path;
        state.file.open(state.path);
        if (!state.file.is_open())
            throw std::runtime_error(std::string("oestream: failed to open file: ") + file_path);
        state.currentTime = TIME("00:00:00");
    }

    struct state_type {
        const char *path = nullptr;
        std::ofstream file;
        bool prop        = false;
        TIME currentTime = TIME();
    };
    state_type state;

    using input_ports  = std::tuple<typename defs::in>;
    using output_ports = std::tuple<>;

    void internal_transition() {}

    void external_transition(TIME e, typename cadmium::make_message_bags<input_ports>::type mbs) {
        state.currentTime += e;
        for (const auto &x : cadmium::get_messages<typename defs::in>(mbs))
            state.file << state.currentTime << " " << x << "\n";
    }

    void confluence_transition(TIME, typename cadmium::make_message_bags<input_ports>::type) {}

    typename cadmium::make_message_bags<output_ports>::type output() const {
        return typename cadmium::make_message_bags<output_ports>::type{};
    }

    TIME time_advance() const {
        return std::numeric_limits<TIME>::infinity();
    }

    friend std::ostringstream &
    operator<<(std::ostringstream &os,
               const typename oestream_output<MSG, TIME, PORT_TYPE>::state_type &) {
        os << "";
        return os;
    }
};

#endif // CADMIUM_OESTREAM_HPP
