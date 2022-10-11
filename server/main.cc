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
#include <execinfo.h>
#include <signal.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <stdlib.h>
#include <unistd.h>

#include <memory>

#include "base/oram_utils.h"
#include "base/oram_config.h"
#include "parse/oram_parse.h"
#include "oram_server.h"

std::shared_ptr<spdlog::logger> logger = spdlog::stdout_color_mt("oram_server");

// Defines an error handler.
void handler(int sig) {
  // Flush the log to capture all error information.
  logger->flush();
  // Print the stack trace.
  void* array[10];
  size_t size;

  // get void*'s for all entries on the stack
  size = backtrace(array, 10);

  // print out all the frames to stderr
  fprintf(stderr, "Error: signal %d:\n", sig);
  backtrace_symbols_fd(array, size, STDERR_FILENO);
  exit(1);
}

int main(int argc, char* argv[]) {
  signal(SIGSEGV, handler);
  signal(SIGABRT, handler);
  signal(SIGINT, handler);

  // Create a parser.
  oram_parse::YamlParser parser;
  oram_impl::OramConfig config;
  // Try configuration file.
  oram_impl::OramStatus status = parser.Parse(config);
  if (status.error_code() == oram_impl::StatusCode::kFileNotFound &&
      !parser.IgnoreCommandLineArgs()) {
    parser.FromCommandLine(argc, argv, config);
  }

  // Initialize the logger.
  spdlog::set_default_logger(logger);
  spdlog::set_level(static_cast<spdlog::level::level_enum>(config.log_level));
  spdlog::flush_every(std::chrono::seconds(config.log_frequency));

  std::unique_ptr<oram_impl::ServerRunner> server_runner =
      std::make_unique<oram_impl::ServerRunner>(
          config.server_address, config.server_port, config.key_path,
          config.crt_path);
  server_runner->Run();

  return 0;
}