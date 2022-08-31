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
#include "ods_client.h"

#include <execinfo.h>
#include <signal.h>

#include <absl/flags/flag.h>
#include <absl/flags/parse.h>
#include <spdlog/fmt/bin_to_hex.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

extern std::shared_ptr<spdlog::logger> logger;

ABSL_FLAG(std::string, address, "localhost", "The address of the server.");
ABSL_FLAG(std::string, port, "1234", "The port of the server.");
ABSL_FLAG(std::string, crt_path, "../key/sslcred.crt",
          "The path of the certificate file.");
// ODS PARAMS.
ABSL_FLAG(uint32_t, odict_size, 1e6, "The size of oblivious dictionary.");
ABSL_FLAG(uint32_t, client_cache_max_size, 32, "The size of cache.");
ABSL_FLAG(uint32_t, x, 2, "The param for padding.");

// ORAM PARAMS.
ABSL_FLAG(uint32_t, block_num, 1e6, "The number of blocks.");
ABSL_FLAG(uint32_t, bucket_size, 4,
          "The size of each bucket. (Z in Path ORAM)");

// Log setting
ABSL_FLAG(int, log_level, spdlog::level::info, "The level of the log.");
// Disable debugging.
ABSL_FLAG(bool, disable_debugging, true, "Hide secret on the server.");

std::shared_ptr<spdlog::logger> logger = spdlog::stdout_color_mt("ods_client");

void Handler(int signal) {
  logger->info("Client stopped.");
  logger->flush();

  // Print the stack trace.
  void* array[10];
  size_t size;

  // get void*'s for all entries on the stack
  size = backtrace(array, 4);

  // print out all the frames to stderr
  fprintf(stderr, "Error: signal %d:\n", signal);
  backtrace_symbols_fd(array, size, STDERR_FILENO);
  exit(1);
}

int main(int argc, char** argv) {
  // Ignore the https proxy for gRPC client.
  unsetenv("https_proxy");

  // Register the signal handler.
  signal(SIGINT, Handler);
  signal(SIGABRT, Handler);

  absl::ParseCommandLine(argc, argv);

  spdlog::set_default_logger(logger);
  spdlog::set_level(
      static_cast<spdlog::level::level_enum>(absl::GetFlag(FLAGS_log_level)));
  spdlog::flush_every(std::chrono::seconds(3));

  std::unique_ptr<Client> client = std::make_unique<Client>(
      absl::GetFlag(FLAGS_address), absl::GetFlag(FLAGS_port),
      absl::GetFlag(FLAGS_crt_path), absl::GetFlag(FLAGS_odict_size),
      absl::GetFlag(FLAGS_client_cache_max_size), absl::GetFlag(FLAGS_x),
      absl::GetFlag(FLAGS_block_num), absl::GetFlag(FLAGS_bucket_size));

  for (size_t i = 0; i < 100; i++) {
    // Insert some nodes.

    auto ret = client->controller_->Insert(std::make_pair(i, std::to_string(i)));
    logger->info("OK");
  }

  for (size_t i = 0; i < 100; i++) {
    // Read some nodes.
    auto ret = client->controller_->Find(i);
    logger->info("OK {}", ret->kv_pair_.second);
  }

  return 0;
}