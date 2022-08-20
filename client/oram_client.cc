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
#include <spdlog/fmt/bin_to_hex.h>

#include "base/oram_utils.h"

extern std::shared_ptr<spdlog::logger> logger;

namespace partition_oram {
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
  stub_ = std::move(server::NewStub(grpc::CreateChannel(address, ssl_creds)));

  // Initialize the cryptor and controller.
  cryptor_ = oram_crypto::Cryptor::GetInstance();
  controller_ = OramController::GetInstance();
  // The stub can be shared between multiple objects.
  controller_->SetStub(stub_);
  controller_->SetBucketSize(bucket_size_);
  controller_->SetNu(3);

  // Test if crypto is working.
  std::string test_str = "Hello, world!";
  std::string hash;
  cryptor_->Digest((uint8_t*)test_str.data(), test_str.size(), &hash);
  logger->info("The hash of {} is {}.", test_str, spdlog::to_hex(hash));
}

int Client::StartKeyExchange(void) {
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
  Status oram_status;
  if ((oram_status = cryptor_->SampleSessionKey(response.public_key_server(),
                                                0)) != Status::kOK) {
    logger->error("Failed to sample session key! Error: {}",
                  kErrorList.at(oram_status));
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

int Client::SendHello(void) {
  const std::string initial_message = "Hello";
  std::string message;
  uint8_t* const iv = (uint8_t*)malloc(ORAM_CRYPTO_RANDOM_SIZE);
  cryptor_->Encrypt((uint8_t*)initial_message.data(), initial_message.size(),
                    iv, &message);
  logger->info("The message is {}.", spdlog::to_hex(message));

  grpc::ClientContext context;
  HelloMessage request;
  google::protobuf::Empty empty;
  request.set_content(message);
  request.set_iv(iv, ORAM_CRYPTO_RANDOM_SIZE);

  oram_utils::SafeFree(iv);

  grpc::Status status = stub_->SendHello(&context, request, &empty);

  if (!status.ok()) {
    logger->error(status.error_message());
    return -1;
  }

  logger->info("The message sent and sucessfully verified by the server.");

  return 0;
}

int Client::ResetServer(void) {
  grpc::ClientContext context;
  google::protobuf::Empty empty;

  grpc::Status status = stub_->ResetServer(&context, empty, &empty);

  if (!status.ok()) {
    logger->error(status.error_message());
    return -1;
  }

  logger->info("The server is reset.");

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
  Status oram_status;
  if ((oram_status = controller_->Run(block_num_, bucket_size_)) !=
      Status::kOK) {
    logger->error("Failed to initialize the oram! Error: {}",
                  kErrorList.at(oram_status));
    return -1;
  } else {
    logger->info("The oram is initialized.");
    return 0;
  }
}

int Client::TestOram(void) {
  // controller_->TestPathOram(0);
  controller_->TestPartitionOram();
  return 0;
}
}  // namespace partition_oram