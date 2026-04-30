/**
 * Copyright (c) 2017-present, Damian Vicino, Laouen M. L. Belloli
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

#ifndef CADMIUM_COMMON_LOGGERS_HELPERS_HPP
#define CADMIUM_COMMON_LOGGERS_HELPERS_HPP

#include <concepts>
#include <iostream>
#include <sstream>
#include <string>
#include <tuple>
#include <typeinfo>
#include <vector>

#include <cadmium/modeling/message_bag.hpp>

namespace cadmium {
namespace logger {

// ── item 4: replace is_streamable SFINAE with a concept ──────────────

template <typename T>
concept Streamable = requires(std::ostream &os, const T &v) { os << v; };

// Print a value if it supports operator<<, otherwise fall back to the
// type's typeid name.
template <typename T> void print_value_or_name(std::ostream &os, const T &v) {
  if constexpr (Streamable<T>)
    os << v;
  else
    os << "obscure message of type " << typeid(T).name();
}

// ── item 6: stable model-type name ────────────────────────────────────

// Returns T::model_name() if the model provides it as a static method,
// otherwise falls back to typeid(T).name() (implementation-defined,
// but at least in a single place that can be updated later).
template <typename T> std::string model_type_name() {
  if constexpr (requires {
                  { T::model_name() } -> std::convertible_to<std::string>;
                })
    return T::model_name();
  else
    return typeid(T).name();
}

// ── Formatting helpers (unchanged interface, updated internals) ────────

template <typename T>
std::ostream &implode(std::ostream &os, const T &collection) {
  os << "{";
  auto it = std::begin(collection);
  if (it != std::end(collection)) {
    print_value_or_name(os, *it);
    ++it;
  }
  while (it != std::end(collection)) {
    os << ", ";
    print_value_or_name(os, *it);
    ++it;
  }
  os << "}";
  return os;
}

template <typename T>
std::vector<std::string> messages_as_strings(const T &collection) {
  std::vector<std::string> ret;
  std::ostringstream oss;
  for (const auto &item : collection) {
    print_value_or_name(oss, item);
    ret.push_back(oss.str());
    oss.str("");
    oss.clear();
  }
  return ret;
}

template <std::size_t S, typename... T> struct print_messages_by_port_impl {
  using current_bag = std::tuple_element_t<S - 1, std::tuple<T...>>;
  static void run(std::ostream &os, const std::tuple<T...> &b) {
    print_messages_by_port_impl<S - 1, T...>::run(os, b);
    os << ", " << typeid(typename current_bag::port).name() << ": ";
    implode(os, cadmium::get_messages<typename current_bag::port>(b));
  }
};

template <typename... T> struct print_messages_by_port_impl<1, T...> {
  using current_bag = std::tuple_element_t<0, std::tuple<T...>>;
  static void run(std::ostream &os, const std::tuple<T...> &b) {
    os << typeid(typename current_bag::port).name() << ": ";
    implode(os, cadmium::get_messages<typename current_bag::port>(b));
  }
};

template <typename... T> struct print_messages_by_port_impl<0, T...> {
  static void run(std::ostream &, const std::tuple<T...> &) {}
};

template <typename... T>
void print_messages_by_port(std::ostream &os, const std::tuple<T...> &b) {
  os << "[";
  print_messages_by_port_impl<sizeof...(T), T...>::run(os, b);
  os << "]";
}

} // namespace logger
} // namespace cadmium

#endif // CADMIUM_COMMON_LOGGERS_HELPERS_HPP
