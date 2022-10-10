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
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include "client/oram_client.h"
#include "core/oram.h"
#include "parse/oram_parse.h"

std::shared_ptr<spdlog::logger> logger = spdlog::stdout_color_mt("oram_client");

int main(int argc, char* argv[]) {
  // Create a parser.
  oram_parse::YamlParser parser;
  oram_impl::OramConfig config;
  // Try configuration file.
  oram_impl::OramStatus status = parser.Parse(config);
  if (status.ErrorCode() == oram_impl::StatusCode::kFileNotFound &&
      !parser.IgnoreCommandLineArgs()) {
    parser.FromCommandLine(argc, argv, config);
  }

  // Control the log level.
  logger->set_level(static_cast<spdlog::level::level_enum>(config.log_level));
  spdlog::set_default_logger(logger);

  // Create the controller.
  std::unique_ptr<oram_impl::OramClient> client =
      std::make_unique<oram_impl::OramClient>(config);
  client->Ready();

  status = client->FillWithData();
  if (!status.ok()) {
    logger->error("Client: FillWithData failed due to `{}`.",
                  status.ErrorMessage());
    abort();
  }

  for (size_t i = 0; i < config.block_num; i++) {
    oram_impl::oram_block_t block;
    block.header.block_id = i;
    oram_impl::OramStatus status = oram_impl::OramStatus::OK;

    if (!(status = client->Read(i, &block)).ok()) {
      logger->error("[-] Error reading block `{}` due to `{}`.", i,
                    status.ErrorMessage());
    } else {
      logger->info("[+] {}: {}", i, block.data[0]);
    }
  }

  return 0;
}