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
#include "oram_client.h"

#include <spdlog/spdlog.h>

extern std::shared_ptr<spdlog::logger> logger;

namespace oram_impl {

static std::shared_ptr<oram_server::Stub> CreateStub(
    const std::string& address, uint32_t port, const std::string& crt_path) {
  const std::string full_address = oram_utils::StrCat(address, ":", port);

  // Configure the SSL connection.
  const std::string crt_file = oram_utils::ReadKeyCrtFile(crt_path);
  grpc::SslCredentialsOptions ssl_opts;
  ssl_opts.pem_root_certs = crt_file;
  std::shared_ptr<grpc::ChannelCredentials> ssl_creds =
      grpc::SslCredentials(ssl_opts);

  // Make this stub shared among all.
  return oram_server::NewStub(grpc::CreateChannel(full_address, ssl_creds));
}

OramClient::OramClient(const OramConfig& config) : config_(config) {
  std::shared_ptr<oram_server::Stub> stub;

  // Check if the proxy should be enabled.
  // If the `enable_proxy` is set to `true`, then the user may need to manaully
  // configure the envoy proxy server in `envoy.yaml`.
  if (!config.enable_proxy) {
    stub = std::move(
        CreateStub(config.server_address, config.server_port, config.crt_path));

    // Unset it.
    unsetenv("https_proxy");
  } else {
    stub = std::move(
        CreateStub(config.proxy_address, config.proxy_port, config.crt_path));
  }

  // Initialize the cryptor.
  oram_crypto::Cryptor::GetInstance();

  // Initialize the oram controller.
  switch (config.oram_type) {
    case OramType::kLinearOram: {
      oram_controller_ = std::make_unique<LinearOramController>(
          config.id, true, config.block_num);
      break;
    }
    case OramType::kPathOram: {
      oram_controller_ = std::make_unique<PathOramController>(
          config.id, config.block_num, config.bucket_size, true);
      break;
    }
    case OramType::kPartitionOram: {
      oram_controller_ = std::move(PartitionOramController::GetInstance());
      break;
    }
    default: {
      logger->error("[-] This type is currently fully implemented.");

      abort();
    }
  }

  // Set the stub.
  oram_controller_->SetStub(stub);

  // Initialize this oram controller.
  OramStatus status = OramStatus::OK;
  if (!(status = oram_controller_->InitOram()).ok()) {
    logger->error("[-] Unable to initialize {} because {}.",
                  oram_controller_->GetName(), status.ErrorMessage());

    abort();
  }

  // TODO: Read data from some file.
}

OramStatus OramClient::Ready(void) {
  auto cryptor_ = oram_crypto::Cryptor::GetInstance();

  if (config_.disable_debugging) {
    cryptor_->NoNeedForSessionKey();
    return OramStatus::OK;
  } else {
    return oram_controller_->KeyNegotiate();
  }
}

OramStatus OramClient::FillWithData(void) {
  // There are two ways to fill the ORAM with data.
  // 1. Randomly sample some garbaga data.
  // 2. Read data from a given file.

  // Check if the filepath is valid.
  if (!config_.filepath.empty()) {
    // Read from file.
    return oram_controller_->FromFile(config_.filepath);
  } else {
    // Sample some random data.
    const size_t block_num = config_.block_num;
    std::vector<oram_block_t> blocks;

    // Should check if the current one is TreeOram.
    if (oram_controller_->GetOramType() == OramType::kPathOram) {
      PathOramController* const path_oram_controller =
          oram_utils::TryCast<OramController, PathOramController>(
              oram_controller_.get());

      const size_t level = path_oram_controller->GetTreeLevel();
      const size_t tree_size = (POW2(level + 1) - 1) * config_.bucket_size;

      oram_utils::SampleRandomBucket(block_num, tree_size, 0ul);
    } else {
      oram_utils::SampleRandomBucket(block_num, block_num, 0ul);
    }

    return oram_controller_->FillWithData(blocks);
  }
}
}  // namespace oram_impl