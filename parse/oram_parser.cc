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
#include "oram_parser.h"

#include <cstdlib>
#include <filesystem>

#include <spdlog/logger.h>

#include "base/oram_utils.h"

extern std::shared_ptr<spdlog::logger> logger;

namespace oram_parse {

oram_impl::OramStatus YamlParser::DoParse(const YAML::const_iterator& cur_iter,
                                          oram_impl::OramConfig& config) {
  const std::string key = cur_iter->first.as<std::string>();

  logger->info("[Config] {}: {}", key, cur_iter->second.as<std::string>());

  // Check the name.
  if (key == "OramType") {
    return oram_utils::TryExec([&]() {
      config.oram_type =
          oram_utils::StrToType(cur_iter->second.as<std::string>());
    });

  } else if (key == "BlockNum") {
    return oram_utils::TryExec(
        [&]() { config.block_num = cur_iter->second.as<size_t>(); });
  } else if (key == "BucketSize") {
    return oram_utils::TryExec(
        [&]() { config.bucket_size = cur_iter->second.as<size_t>(); });

  } else if (key == "ServerCrtPath") {
    return oram_utils::TryExec(
        [&]() { config.crt_path = cur_iter->second.as<std::string>(); });

  } else if (key == "ServerKeyPath") {
    return oram_utils::TryExec(
        [&]() { config.key_path = cur_iter->second.as<std::string>(); });

  } else if (key == "ServerAddress") {
    return oram_utils::TryExec(
        [&]() { config.server_address = cur_iter->second.as<std::string>(); });

  } else if (key == "EnableProxy") {
    return oram_utils::TryExec(
        [&]() { config.enable_proxy = cur_iter->second.as<bool>(); });

  } else if (key == "ProxyAddress") {
    return oram_utils::TryExec(
        [&]() { config.proxy_address = cur_iter->second.as<std::string>(); });

  } else if (key == "Port") {
    return oram_utils::TryExec(
        [&]() { config.port = cur_iter->second.as<uint32_t>(); });

  } else if (key == "OdictSize") {
    return oram_utils::TryExec(
        [&]() { config.odict_size = cur_iter->second.as<size_t>(); });

  } else if (key == "ClientCacheMaxSize") {
    return oram_utils::TryExec([&]() {
      config.client_cache_max_size = cur_iter->second.as<size_t>();
    });

  } else {
    return oram_impl::OramStatus(
        oram_impl::StatusCode::kInvalidArgument,
        oram_utils::StrCat("Unknown config key: `", key, "`."));
  }
}

oram_impl::OramStatus YamlParser::Parse(oram_impl::OramConfig& config) {
  oram_impl::OramStatus status = oram_impl::OramStatus::OK;

  // Get default configuration.
  config = oram_impl::default_config;

  // Check the environment variable.
  std::filesystem::path config_path = default_config_path;

  if (std::getenv("ORAM_CONFIG_PATH") != nullptr) {
    config_path = std::getenv("ORAM_CONFIG_PATH");
  }

  // Check if such a path exists or is a directory.
  if (!std::filesystem::exists(config_path) ||
      std::filesystem::is_directory(config_path)) {
    // Fallback and check.
    logger->warn("The path does not exist!. Fall back to `{}`",
                 default_config_path);

    if (!std::filesystem::exists(default_config_path) ||
        std::filesystem::is_directory(default_config_path)) {
      logger->warn("Default path not exists! Accepting command line argument.",
                   default_config_path);

      return oram_impl::OramStatus::OK;
    }
  }

  // Configuration file detected.
  ignore_command_line_args_ = true;

  // If is this is yaml file?
  std::string ext = config_path.filename().string();
  ext = ext.substr(ext.find_last_of(".") + 1);

  if (ext != "yaml" && ext != "yml") {
    logger->error(
        "`{}` is not a yaml file. Perhaps you have set `ORAM_CONFIG_PATH` "
        "environment variable to a wrong value?",
        config_path.string());

    return oram_impl::OramStatus(oram_impl::StatusCode::kFileIOError,
                                 "Not a yaml file.");
  }

  // Read the file.
  const YAML::Node node = YAML::LoadFile(config_path);

  // Visit all the node.
  for (YAML::const_iterator it = node.begin(); it != node.end(); ++it) {
    status = DoParse(it, config);
    if (!status.ok()) {
      logger->error(status.ErrorMessage());

      abort();
    }
  }

  return status;
}

}  // namespace oram_parse