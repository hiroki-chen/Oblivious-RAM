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
oram_impl::OramConfig YamlParser::EmitDefault(void) {
  return oram_impl::OramConfig();
}

oram_impl::OramStatus YamlParser::Parse(oram_impl::OramConfig& config) {
  oram_impl::OramStatus status = oram_impl::OramStatus::OK;

  // Check the environment variable.
  std::filesystem::path config_path = default_config_path;

  if (std::getenv("ORAM_CONFIG_PATH") != nullptr) {
    config_path = std::getenv("ORAM_CONFIG_PATH");
  }

  // Check if such a path exists or is a directory.
  if (!std::filesystem::exists(config_path) ||
      std::filesystem::is_directory(config_path)) {
    // Fallback and check.
    if (!std::filesystem::exists(default_config_path) ||
        std::filesystem::is_directory(default_config_path)) {
      logger->warn("Default path not exists! Fallback to `{}`",
                   default_config_path);

      // Get default configuration.
      config = EmitDefault();

      return oram_impl::OramStatus::OK;
    }
  }

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
  // Test
  status = oram_utils::TryExec([=]() { node["BlockNum"].as<size_t>(); });
  if (!status.ok()) {
    logger->error("Read configuration file error: {}", status.ErrorMessage());

    // Set to default? TODO: Always construct a default value.
    return status;
  }

  return status;
}

}  // namespace oram_parse