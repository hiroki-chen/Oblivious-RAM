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
#include "oram_server.h"

#include <stdio.h>
#include <execinfo.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <memory>

#include <absl/flags/flag.h>
#include <absl/flags/parse.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include "base/oram_utils.h"

// Configurations for the server.
ABSL_FLAG(std::string, address, "0.0.0.0", "The server's IP address.");
ABSL_FLAG(std::string, port, "1234", "The server's port.");
ABSL_FLAG(std::string, key_path, "../key/sslcred.key",
          "The path to the key file.");
ABSL_FLAG(std::string, crt_path, "../key/sslcred.crt",
          "The path to the certificate file.");
ABSL_FLAG(int, log_level, spdlog::level::info, "The severity of the logger.");

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
  // Parse the command line arguments.
  absl::ParseCommandLine(argc, argv);

  // Initialize the logger.
  spdlog::set_default_logger(logger);
  spdlog::set_level(
      static_cast<spdlog::level::level_enum>(absl::GetFlag(FLAGS_log_level)));
  spdlog::flush_every(std::chrono::seconds(3));

  std::unique_ptr<partition_oram::ServerRunner> server_runner =
      std::make_unique<partition_oram::ServerRunner>(
          absl::GetFlag(FLAGS_address), absl::GetFlag(FLAGS_port),
          absl::GetFlag(FLAGS_key_path), absl::GetFlag(FLAGS_crt_path));
  server_runner->Run();

  return 0;
}