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
#include <memory>

#include <signal.h>
#include <execinfo.h>

#include <absl/flags/flag.h>
#include <absl/flags/parse.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include "oram_client.h"

ABSL_FLAG(std::string, address, "localhost", "The address of the server.");
ABSL_FLAG(std::string, port, "1234", "The port of the server.");
ABSL_FLAG(std::string, crt_path, "../key/sslcred.crt",
          "The path of the certificate file.");

// ORAM PARAMS.
ABSL_FLAG(uint32_t, block_num, 1e6, "The number of blocks.");
ABSL_FLAG(uint32_t, bucket_size, 4,
          "The size of each bucket. (Z in Path ORAM)");

// Log setting
ABSL_FLAG(int, log_level, spdlog::level::info, "The level of the log.");

// Start experiment series.
ABSL_FLAG(bool, start_experiment, false, "Start experiment series.");

std::shared_ptr<spdlog::logger> logger = spdlog::stdout_color_mt("oram_client");

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

  std::unique_ptr<partition_oram::Client> client =
      std::make_unique<partition_oram::Client>(
          absl::GetFlag(FLAGS_address), absl::GetFlag(FLAGS_port),
          absl::GetFlag(FLAGS_crt_path), absl::GetFlag(FLAGS_bucket_size),
          absl::GetFlag(FLAGS_block_num));
  client->Run();
  client->StartKeyExchange();

  if (absl::GetFlag(FLAGS_start_experiment)) {
    for (size_t i = 6; i <= 21; i++) {
      const size_t block_num = POW2(i);

      logger->info("------------------------------");
      logger->info("Start experiment with block_num = {}", block_num);
      client->InitOram();
      client->TestOram();
      client->ResetServer();
      client->Reset(block_num);
      logger->info("-----------------------------");
    }
    client->CloseConnection();
  } else {
    // client->SendHello();
    client->InitOram();
    client->TestOram();

    client->CloseConnection();
  }

  return 0;
}