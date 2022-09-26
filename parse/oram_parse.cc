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
#include "oram_parse.h"

#include <cstdlib>
#include <filesystem>

#include <absl/flags/flag.h>
#include <absl/flags/parse.h>
#include <spdlog/logger.h>

#include "base/oram_utils.h"

// Register for ABSL_FLAGS.
ABSL_FLAG(std::string, server_address, "0.0.0.0", "The address of the server.");
ABSL_FLAG(uint16_t, server_port, 1234, "The port of the server.");
ABSL_FLAG(std::string, crt_path, "./key/server.crt",
          "The path to the certificate file of the server.");
ABSL_FLAG(std::string, key_path, "./key/server.key",
          "The path to the key file of the server.");

ABSL_FLAG(bool, enable_proxy, false, "Should we enable proxy or not.");
ABSL_FLAG(std::string, proxy_address, "", "The address of the proxy server.");
ABSL_FLAG(uint32_t, proxy_port, 0, "The port of the proxy server.");

ABSL_FLAG(std::string, oram_type, "PathOram",
          "The type of the ORAM controller.");
ABSL_FLAG(uint32_t, id, 0, "The ID for the current ORAM controller.");
ABSL_FLAG(uint32_t, block_num, 1e5, "The number of the block.");
ABSL_FLAG(uint32_t, bucket_size, 4,
          "The size of each bucket. (Z in Path ORAM)");

ABSL_FLAG(uint32_t, odict_size, 1e5, "The size of the oblivious dictionary.");
ABSL_FLAG(uint32_t, client_cache_size, 32, "The size of the client cache.");

// Log settings.
ABSL_FLAG(uint32_t, log_level, 2, "The level of the log.");
ABSL_FLAG(uint32_t, log_frequency, 3,
          "The time interval when the log is flushed.");

// Disable debugging.
ABSL_FLAG(bool, disable_debugging, true, "Hide secret on the server.");

// Data source.
ABSL_FLAG(std::string, file_path, "",
          "The path to the file that stores the ORAM data.");

namespace oram_parse {

oram_impl::OramStatus YamlParser::DoParse(const YAML::const_iterator& cur_iter,
                                          oram_impl::OramConfig& config) {
  const std::string key = cur_iter->first.as<std::string>();

  logger_->info("[Config] {}: {}", key, cur_iter->second.as<std::string>());

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

  } else if (key == "Id") {
    return oram_utils::TryExec(
        [&]() { config.crt_path = cur_iter->second.as<uint32_t>(); });

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

  } else if (key == "ServerPort") {
    return oram_utils::TryExec(
        [&]() { config.server_port = cur_iter->second.as<uint32_t>(); });

  } else if (key == "ProxyPort") {
    return oram_utils::TryExec(
        [&]() { config.proxy_port = cur_iter->second.as<uint32_t>(); });

  } else if (key == "LogLevel") {
    return oram_utils::TryExec(
        [&]() { config.log_level = cur_iter->second.as<uint8_t>(); });

  } else if (key == "LogFrequency") {
    return oram_utils::TryExec(
        [&]() { config.log_frequency = cur_iter->second.as<uint8_t>(); });

  } else if (key == "OdictSize") {
    return oram_utils::TryExec(
        [&]() { config.odict_size = cur_iter->second.as<size_t>(); });

  } else if (key == "ClientCacheMaxSize") {
    return oram_utils::TryExec([&]() {
      config.client_cache_max_size = cur_iter->second.as<size_t>();
    });

  } else if (key == "FilePath") {
    return oram_utils::TryExec([&]() {
      config.filepath = cur_iter->second.as<std::string>();
    });

  } else if (key == "DisableDebugging") {
    return oram_utils::TryExec([&]() {
      config.disable_debugging = cur_iter->second.as<bool>();
    });

  } else {
    return oram_impl::OramStatus(
        oram_impl::StatusCode::kInvalidArgument,
        oram_utils::StrCat("Unknown configuration key: `", key, "`."));
  }
}

oram_impl::OramStatus YamlParser::DoCommandLine(oram_impl::OramConfig& config) {
  config.oram_type = oram_utils::StrToType(absl::GetFlag(FLAGS_oram_type));
  config.block_num = absl::GetFlag(FLAGS_block_num);
  config.bucket_size = absl::GetFlag(FLAGS_bucket_size);
  config.id = absl::GetFlag(FLAGS_id);
  config.crt_path = absl::GetFlag(FLAGS_crt_path);
  config.key_path = absl::GetFlag(FLAGS_key_path);
  config.server_address = absl::GetFlag(FLAGS_server_address);
  config.server_port = absl::GetFlag(FLAGS_server_port);
  config.enable_proxy = absl::GetFlag(FLAGS_enable_proxy);
  config.proxy_address = absl::GetFlag(FLAGS_proxy_address);
  config.proxy_port = absl::GetFlag(FLAGS_proxy_port);
  config.log_level = absl::GetFlag(FLAGS_log_level);
  config.log_frequency = absl::GetFlag(FLAGS_log_frequency);
  config.odict_size = absl::GetFlag(FLAGS_odict_size);
  config.client_cache_max_size = absl::GetFlag(FLAGS_client_cache_size);
  config.disable_debugging = absl::GetFlag(FLAGS_disable_debugging);
  config.filepath = absl::GetFlag(FLAGS_file_path);

  return oram_impl::OramStatus::OK;
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
    logger_->warn("The path does not exist!. Fall back to `{}`",
                  default_config_path);

    if (!std::filesystem::exists(default_config_path) ||
        std::filesystem::is_directory(default_config_path)) {
      logger_->warn("Default path not exists! Accepting command line argument.",
                    default_config_path);

      return oram_impl::OramStatus(oram_impl::StatusCode::kFileNotFound,
                                   "No configuration file provided.");
    }
  }

  // Configuration file detected.
  ignore_command_line_args_ = true;

  // If is this is yaml file?
  std::string ext = config_path.filename().string();
  ext = ext.substr(ext.find_last_of(".") + 1);

  if (ext != "yaml" && ext != "yml") {
    logger_->error(
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
      logger_->error(status.ErrorMessage());

      abort();
    }
  }

  return status;
}

oram_impl::OramStatus YamlParser::FromCommandLine(
    int argc, char** argv, oram_impl::OramConfig& config) {
  // Check if we have already provided the valid configuration file.
  if (ignore_command_line_args_) {
    logger_->warn(
        "[!] Detected configuration file, so command line arguments will be "
        "IGNORED!");
    oram_impl::OramStatus(oram_impl::StatusCode::kAlreadyUsed,
                          "Command line arguments are overriden.");
  }

  absl::ParseCommandLine(argc, argv);

  return DoCommandLine(config);
}

YamlParser::YamlParser()
    : logger_(spdlog::stderr_color_mt("parser")),
      ignore_command_line_args_(false) {
  spdlog::flush_every(std::chrono::seconds(3));
}
}  // namespace oram_parse