/**
 * Copyright (c) 2018-present, Laouen M. L. Belloli
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

#ifndef CADMIUM_DYNAMIC_COMMON_LOGGERS_HPP
#define CADMIUM_DYNAMIC_COMMON_LOGGERS_HPP

#include <string>
#include <vector>

namespace cadmium {
    namespace dynamic {
        namespace logger {

            // Data returned by link::route_messages — carries routing metadata for logging.
            struct routed_messages {
                std::string from_port;
                std::string to_port;
                std::vector<std::string> from_messages;
                std::vector<std::string> to_messages;

                routed_messages() = default;

                routed_messages(std::string from_p, std::string to_p)
                    : from_port(std::move(from_p)), to_port(std::move(to_p)) {}

                routed_messages(std::vector<std::string> from_msgs,
                                std::vector<std::string> to_msgs, std::string from_p,
                                std::string to_p)
                    : from_port(std::move(from_p)), to_port(std::move(to_p)),
                      from_messages(std::move(from_msgs)), to_messages(std::move(to_msgs)) {}
            };

        } // namespace logger
    } // namespace dynamic
} // namespace cadmium

#endif // CADMIUM_DYNAMIC_COMMON_LOGGERS_HPP
