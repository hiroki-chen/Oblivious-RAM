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

#include <atomic>
#include <thread>

#include <spdlog/fmt/bin_to_hex.h>

#include "base/oram_utils.h"
#include "base/oram_defs.h"

std::atomic_bool server_running;

namespace partition_oram {

grpc::Status PartitionORAMService::InitOram(grpc::ServerContext* context,
                                            const InitOramRequest* request,
                                            google::protobuf::Empty* empty) {
  logger->info("From peer: {}, InitOram request received.", context->peer());

  // Intialize the Path ORAM storage.
  const uint32_t id = request->id();
  const uint32_t bucket_size = request->bucket_size();
  const uint32_t bucket_num = request->bucket_num();

  // Do some sanity check.
  if (id < storages_.size()) {
    const std::string error_message =
        oram_utils::StrCat("PathORAM id: ", id, " already exists.");
    return grpc::Status(grpc::StatusCode::ALREADY_EXISTS, error_message);
  } else if (id >= kMaximumOramStorageNum) {
    const std::string error_message = oram_utils::StrCat(
        "PathORAM id: ", id, " exceeds the maximum number of ORAM storages: ",
        kMaximumOramStorageNum);
    return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, error_message);
  }

  // Create a new storage and initialize it.
  storages_.emplace_back(
      std::make_unique<OramServerStorage>(id, bucket_num, bucket_size));

  logger->info("PathORAM id: {} successfully created.", id);

  return grpc::Status::OK;
}

grpc::Status PartitionORAMService::ResetServer(grpc::ServerContext* context,
                         const google::protobuf::Empty* request,
                         google::protobuf::Empty* response) {
  logger->info("From peer: {}, Reset server.", context->peer());

  storages_.clear();
  cryptor_.reset();

  return grpc::Status::OK;
}

grpc::Status PartitionORAMService::PrintOramTree(
    grpc::ServerContext* context, const PrintOramTreeRequest* request,
    google::protobuf::Empty* response) {
  logger->info("From peer: {}, PrintOramTree request received.",
               context->peer());

  const uint32_t id = request->id();

  if (id >= storages_.size()) {
    const std::string error_message =
        oram_utils::StrCat("PathORAM id: ", id, " does not exist.");
    return grpc::Status(grpc::StatusCode::NOT_FOUND, error_message);
  }

  oram_utils::PrintOramTree(std::move(storages_[id]->get_storage()));

  return grpc::Status::OK;
}

grpc::Status PartitionORAMService::ReadPath(grpc::ServerContext* context,
                                            const ReadPathRequest* request,
                                            ReadPathResponse* response) {
  logger->info("From peer: {}, ReadPath request received.", context->peer());

  const uint32_t id = request->id();
  const uint32_t path = request->path();
  const uint32_t level = request->level();

  logger->info("PathORAM id: {}, path: {}, level: {}", id, path, level);

  if (id >= storages_.size()) {
    const std::string error_message =
        oram_utils::StrCat("PathORAM id: ", id, " does not exist.");
    return grpc::Status(grpc::StatusCode::NOT_FOUND, error_message);
  }

  // Read the path and record the time it used.
  auto begin = std::chrono::high_resolution_clock::now();

  p_oram_bucket_t bucket;
  if (storages_[id]->ReadPath(level, path, &bucket) != Status::kOK) {
    const std::string error_message =
        oram_utils::StrCat("Failed to read path: ", path, " in level: ", level,
                           " in PathORAM id: ", id);
    return grpc::Status(grpc::StatusCode::INTERNAL, error_message);
  }
  auto end = std::chrono::high_resolution_clock::now();
  logger->info(
      "Elapsed time when reading a path: {} us",
      std::chrono::duration_cast<std::chrono::microseconds>(end - begin)
          .count());

  logger->debug("After read path:");
  oram_utils::PrintStash(bucket);

  // Serialze them to the string and send back to the client.
  const std::vector<std::string> serialized_bucket =
      std::move(oram_utils::SerializeToStringVector(bucket));
  // Copy the vector to the response.
  for (const auto& bucket : serialized_bucket) {
    response->add_bucket(bucket);
  }

  return grpc::Status::OK;
}

grpc::Status PartitionORAMService::WritePath(grpc::ServerContext* context,
                                             const WritePathRequest* request,
                                             WritePathResponse* response) {
  logger->info("From peer: {}, WritePath request received.", context->peer());
  Type type = request->type();

  const uint32_t id = request->id();
  const uint32_t level = request->level();
  const uint32_t path = request->path();
  const uint32_t offset = request->offset();

  if (id >= storages_.size()) {
    const std::string error_message =
        oram_utils::StrCat("PathORAM id: ", id, " does not exist.");
    return grpc::Status(grpc::StatusCode::NOT_FOUND, error_message);
  }

  // Deserialize the bucket from the string.
  p_oram_bucket_t bucket = std::move(
      oram_utils::DeserializeFromStringVector(std::vector<std::string>(
          request->bucket().begin(), request->bucket().end())));

  logger->debug("After deserialize:");
  oram_utils::PrintStash(bucket);

  // Write the path.
  Status status =
      type == Type::kInit
          ? storages_[id]->AccurateWritePath(level, offset, bucket, type)
          : storages_[id]->WritePath(level, path, bucket);

  if (status != Status::kOK) {
    const std::string error_message =
        oram_utils::StrCat("Failed to write path: ", path, " in level: ", level,
                           " in PathORAM id: ", id);
    return grpc::Status(grpc::StatusCode::INTERNAL, error_message);
  }

  return grpc::Status::OK;
}

grpc::Status PartitionORAMService::KeyExchange(
    grpc::ServerContext* context, const KeyExchangeRequest* request,
    KeyExchangeResponse* response) {
  const std::string public_key_client = request->public_key_client();

  Status status;
  if ((status = cryptor_->SampleKeyPair()) != Status::kOK) {
    const std::string error_message = oram_utils::StrCat(
        "Failed to sample key pair! Error: ", kErrorList.at(status));
    return grpc::Status(grpc::StatusCode::INTERNAL, error_message);
  }

  if ((status = cryptor_->SampleSessionKey(public_key_client, 1)) !=
      Status::kOK) {
    const std::string error_message = oram_utils::StrCat(
        "Failed to sample session key! Error: ", kErrorList.at(status));
    return grpc::Status(grpc::StatusCode::INTERNAL, error_message);
  }
  auto key_pair = cryptor_->GetKeyPair();
  response->set_public_key_server(key_pair.first);

  auto session_key = std::move(cryptor_->GetSessionKeyPair());
  logger->info("The session key for receiving is {}.",
               spdlog::to_hex(session_key.first));
  logger->info("The session key for sending is {}.",
               spdlog::to_hex(session_key.second));

  return grpc::Status::OK;
}

grpc::Status PartitionORAMService::CloseConnection(
    grpc::ServerContext* context, const google::protobuf::Empty* request,
    google::protobuf::Empty* response) {
  logger->info("Closing connection...");
  server_running = false;
  return grpc::Status::OK;
}

grpc::Status PartitionORAMService::SendHello(grpc::ServerContext* context,
                                             const HelloMessage* request,
                                             google::protobuf::Empty* empty) {
  const std::string encrypted_message = request->content();
  const std::string iv = request->iv();
  std::string message;
  Status status;

  logger->info("Received encrypted message: {}.",
               spdlog::to_hex(encrypted_message));

  if ((status = cryptor_->Decrypt((uint8_t*)encrypted_message.data(),
                                  encrypted_message.size(), (uint8_t*)iv.data(),
                                  &message)) != Status::kOK) {
    const std::string error_message = oram_utils::StrCat(
        "Failed to verify Hello message! Error: ", kErrorList.at(status));
    return grpc::Status(grpc::StatusCode::INTERNAL, error_message);
  }

  logger->info("Successfully verified: {}.", message);
  return grpc::Status::OK;
}

grpc::Status PartitionORAMService::ReportServerInformation(
    grpc::ServerContext* context, const google::protobuf::Empty* request,
    google::protobuf::Empty* response) {
  logger->info("Report server information...");

  double storage_size = 0;
  for (const auto& storage : storages_) {
    storage_size += storage->ReportStorage();
  }

  logger->info("The total storage size is {} MB.", storage_size);

  return grpc::Status::OK;
}

ServerRunner::ServerRunner(const std::string& address, const std::string& port,
                           const std::string& key_path,
                           const std::string& crt_path)
    : address_(address), port_(port) {
  const std::string key_file = oram_utils::ReadKeyCrtFile(key_path);
  const std::string crt_file = oram_utils::ReadKeyCrtFile(crt_path);

  if (key_file.empty() || crt_file.empty()) {
    abort();
  }

  // Start to configure the SSL options.
  grpc::SslServerCredentialsOptions::PemKeyCertPair pkcp;
  pkcp.private_key = key_file;
  pkcp.cert_chain = crt_file;

  // Self-signed. NO CA.
  grpc::SslServerCredentialsOptions ssl_opts;
  ssl_opts.pem_root_certs = "";
  ssl_opts.pem_key_cert_pairs.emplace_back(pkcp);
  creds_ = grpc::SslServerCredentials(ssl_opts);

  service_ = std::make_unique<PartitionORAMService>();
  is_initialized = true;
}

void ServerRunner::Run(void) {
  logger->info("Starting server...");

  if (!is_initialized) {
    logger->error("Server not initialized.");
    abort();
  }

  grpc::ServerBuilder builder;
  const std::string address = oram_utils::StrCat(address_, ":", port_);
  builder.AddListeningPort(address, creds_);
  builder.RegisterService(service_.get());

  std::shared_ptr<grpc::Server> server = builder.BuildAndStart();
  logger->info("Server started to listen on {}:{}.", address_, port_);
  server_running = true;

  // Initialize the cryptor.
  service_->cryptor_ = oram_crypto::Cryptor::GetInstance();

  // Start a monitor thread.
  std::thread monitor_thread([&, this]() {
    while (server_running) {
      // Wake up every 100 miliseconds and check if the server is still
      // running.
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    server->Shutdown();
  });
  monitor_thread.detach();

  server->Wait();
}
}  // namespace partition_oram