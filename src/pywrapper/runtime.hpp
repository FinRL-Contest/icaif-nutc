#pragma once

#include <fmt/format.h>
#include <pybind11/embed.h>
#include <pybind11/pybind11.h>

#include <optional>

namespace nutc {
namespace pywrapper {
bool
create_api_module(std::function<bool(const std::string&, int, bool, const std::string&)>
                      publish_market_order);
std::optional<std::string> import_py_code(const std::string& code);

std::optional<std::string> run_py_code(const std::string& code);
} // namespace pywrapper
} // namespace nutc
