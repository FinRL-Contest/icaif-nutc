#pragma once

#include <curl/curl.h>
#include <glaze/glaze.hpp>

#include <iostream>
#include <map>
#include <string>

namespace nutc {
namespace client {

glz::json_t firebase_request(
    const std::string& method, const std::string& url, const std::string& data = ""
);

} // namespace client
} // namespace nutc
