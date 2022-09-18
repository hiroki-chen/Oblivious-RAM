/*
 Copyright (c) 2022 Haobin Chen

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef ORAM_IMPL_BASE_ORAM_CONFIG_H_
#define ORAM_IMPL_BASE_ORAM_CONFIG_H_

#include <cstddef>
#include <string>

#include "base/oram_defs.h"

static const std::string default_config_path = "./config.yaml";

namespace oram_impl {
// Read from the file or command line.
struct OramConfig {
  OramType oram_type;

  size_t block_num;
  size_t bucket_size;

  // For SSL configuration.
  std::string crt_path;
  std::string key_path;

  // For server-client connection.
  std::string server_address;
  bool enable_proxy;
  std::string proxy_address;
  uint32_t port;

  // For ODS.
  size_t odict_size;
  size_t client_cache_max_size;

  bool disable_debugging;
};

static const OramConfig default_config = {
    OramType::kPathOram,
    100000,
    4,

    "./key/server.crt",
    "./key/server.key",

    "0.0.0.0",
    false,
    "",
    1234,

    100000,
    32,

    false,
};
}  // namespace oram_impl

#endif  // ORAM_IMPL_BASE_ORAM_CONFIG_H_