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
#include "linear_oram_client.h"

#include <absl/flags/flag.h>
#include <spdlog/fmt/bin_to_hex.h>
#include <spdlog/spdlog.h>

#include "base/oram_utils.h"

extern std::shared_ptr<spdlog::logger> logger;

void Client::Run(void) {
  logger->info("Client started, and the address is given as {}:{}.",
               server_address_, server_port_);

  std::string address;
  address = oram_utils::StrCat(server_address_, ":", server_port_);

  // Configure the SSL connection.
  const std::string crt_file = oram_utils::ReadKeyCrtFile(crt_path_);
  grpc::SslCredentialsOptions ssl_opts;
  ssl_opts.pem_root_certs = crt_file;
  std::shared_ptr<grpc::ChannelCredentials> ssl_creds =
      grpc::SslCredentials(ssl_opts);
  // Make this stub shared among all.
  stub_ =
      std::move(oram_server::NewStub(grpc::CreateChannel(address, ssl_creds)));

  // Initialize the cryptor and controller.
  cryptor_ = oram_crypto::Cryptor::GetInstance();

  controller_ = std::make_unique<LinearOramController>(0, true, block_num_);
  controller_->SetStub(stub_);
}

int Client::StartKeyExchange(bool disable_debugging) {
  if (disable_debugging) {
    cryptor_->NoNeedForSessionKey();
    return 0;
  }

  cryptor_->SampleKeyPair();
  auto key_pair = std::move(cryptor_->GetKeyPair());

  // Send the public key to the server.
  grpc::ClientContext context;
  KeyExchangeRequest request;
  KeyExchangeResponse response;
  request.set_public_key_client(key_pair.first);

  grpc::Status status = stub_->KeyExchange(&context, request, &response);

  if (!status.ok()) {
    logger->error(status.error_message());
  }
  const std::string public_key_server = response.public_key_server();
  logger->info("The server's public key is {}.",
               spdlog::to_hex(public_key_server));

  // Sample the session key based on the server's public key.
  OramStatus oram_status;
  if (!(oram_status =
            cryptor_->SampleSessionKey(response.public_key_server(), 0))
           .ok()) {
    logger->error("Failed to sample session key! Error: {}",
                  oram_status.ErrorMessage());
    return -1;
  }

  logger->info("The session key sampled.");
  auto session_key = std::move(cryptor_->GetSessionKeyPair());
  logger->info("The session key for receiving is {}.",
               spdlog::to_hex(session_key.first));
  logger->info("The session key for sending is {}.",
               spdlog::to_hex(session_key.second));

  return 0;
}

int Client::CloseConnection(void) {
  grpc::ClientContext context;
  google::protobuf::Empty empty;
  grpc::Status status = stub_->CloseConnection(&context, empty, &empty);

  if (!status.ok()) {
    logger->error(status.error_message());
    return -1;
  }

  logger->info("The connection is closed.");
  return 0;
}

int Client::InitOram(void) {
  // Initialize the oram via the controller.
  OramStatus oram_status;

  if (!(oram_status = controller_->InitOram()).ok()) {
    logger->error("Failed to initialize the oram! Error: {}",
                  oram_status.ErrorMessage());
    return -1;
  } else {
    // Read from file.
    controller_->FromFile("../data/data.txt");
    logger->info("The oram is initialized.");

    return 0;
  }
}

int Client::TestOram(void) {
  logger->info("[+] Tesing Linear ORAM...");

  for (size_t i = 0; i < controller_->GetBlockNum() >> 1; i++) {
    oram_block_t block;
    memset(&block, 0, ORAM_BLOCK_SIZE);

    OramStatus s;
    if (!(s = controller_->Access(Operation::kRead, i, &block)).ok()) {
      logger->error("[-] Error: {}", s.ErrorMessage());
      abort();
    }

    logger->info("[+] Read {}, {}", block.header.block_id, block.data[0]);
  }

  for (size_t i = 0; i < controller_->GetBlockNum() >> 1; i++) {
    oram_block_t block;
    block.header.block_id = i;
    block.header.type = BlockType::kNormal;
    block.data[0] = 255 - i;

    OramStatus s;
    if (!(s = controller_->Access(Operation::kWrite, i, &block)).ok()) {
      logger->error("[-] Error: {}", s.ErrorMessage());
      abort();
    }

    logger->info("[+] Write {}, {}", block.header.block_id, block.data[0]);
  }

  for (size_t i = 0; i < controller_->GetBlockNum() >> 1; i++) {
    oram_block_t block;
    memset(&block, 0, ORAM_BLOCK_SIZE);

    OramStatus s;
    if (!(s = controller_->Access(Operation::kRead, i, &block)).ok()) {
      logger->error("[-] Error: {}", s.ErrorMessage());
      abort();
    }

    logger->info("[+] Read {}, {}", block.header.block_id, block.data[0]);
  }

  logger->info("[-] End testing Linear ORAM.");
  return 0;
}