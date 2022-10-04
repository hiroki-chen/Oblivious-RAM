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

#include <spdlog/fmt/bin_to_hex.h>

#include <atomic>
#include <thread>

#include "base/oram_defs.h"
#include "base/oram_utils.h"

std::atomic_bool server_running;

namespace oram_impl {
grpc::Status OramService::CheckInitRequest(uint32_t id) {
  if (storages_.find(id) != storages_.end()) {
    const std::string error_message =
        oram_utils::StrCat("ORAM id: ", id, " already exists.");
    return grpc::Status(grpc::StatusCode::ALREADY_EXISTS, error_message);
  } else if (id >= kMaximumOramStorageNum) {
    const std::string error_message = oram_utils::StrCat(
        "PathORAM id: ", id, " exceeds the maximum number of ORAM storages: ",
        kMaximumOramStorageNum);
    return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, error_message);
  }

  return grpc::Status::OK;
}

grpc::Status OramService::CheckIdValid(uint32_t id) {
  if (storages_.find(id) == storages_.end()) {
    const std::string error_message =
        oram_utils::StrCat("ORAM id: ", id, " does not exist.");
    return grpc::Status(grpc::StatusCode::NOT_FOUND, error_message);
  }
  return grpc::Status::OK;
}

grpc::Status OramService::InitTreeOram(grpc::ServerContext* context,
                                       const InitTreeOramRequest* request,
                                       google::protobuf::Empty* empty) {
  logger->info("From peer: {}, InitTreeOram request received.",
               context->peer());

  // Intialize the Tree ORAM storage.
  const uint32_t id = request->header().id();
  const std::string instance_hash = request->header().instance_hash();
  const uint32_t bucket_size = request->bucket_size();
  const uint32_t bucket_num = request->bucket_num();
  const size_t block_size = request->block_size();

  // Do check.
  grpc::Status status = CheckInitRequest(id);
  if (!status.ok()) {
    return status;
  }

  // Create a new storage and initialize it.
  storages_[id] = std::make_unique<TreeOramServerStorage>(
      id, bucket_num, block_size, bucket_size, instance_hash);

  logger->info("Tree ORAM successfully created. ID = {}", id);

  return grpc::Status::OK;
}

grpc::Status OramService::InitFlatOram(grpc::ServerContext* context,
                                       const InitFlatOramRequest* request,
                                       google::protobuf::Empty* empty) {
  logger->info("From peer: {}, InitFlatOram request received.",
               context->peer());

  // Initialize the Flat ORAM storage.
  const uint32_t id = request->header().id();
  const std::string instance_hash = request->header().instance_hash();
  const size_t capacity = request->capacity();
  const size_t block_size = request->block_size();

  // Do check.
  grpc::Status status = CheckInitRequest(id);
  if (!status.ok()) {
    return status;
  }

  storages_[id] = std::make_unique<FlatOramServerStorage>(
      id, capacity, block_size, instance_hash);

  logger->info("Flat ORAM successfully created. ID = {}", id);

  return grpc::Status::OK;
}

grpc::Status OramService::InitSqrtOram(grpc::ServerContext* context,
                                       const InitSqrtOramRequest* request,
                                       google::protobuf::Empty* empty) {
  logger->info("From peer: {}, InitSqrtOram request received.",
               context->peer());

  const uint32_t id = request->header().id();
  const std::string instance_hash = request->header().instance_hash();
  const size_t capacity = request->capacity();
  const size_t block_size = request->block_size();
  const size_t squared_m = request->squared_m();

  grpc::Status status = CheckInitRequest(id);
  if (!status.ok()) {
    return status;
  }

  storages_[id] = std::make_unique<SqrtOramServerStorage>(
      id, capacity, block_size, squared_m, instance_hash);

  logger->info("Sqrt Oram successfully created. ID = {}", id);

  return grpc::Status::OK;
}

grpc::Status OramService::ReadSqrtMemory(grpc::ServerContext* context,
                                         const ReadSqrtRequest* request,
                                         SqrtMessage* response) {
  logger->info("From peer: {}, ReadSqrtMemory request received.",
               context->peer());

  const uint32_t id = request->header().id();
  const std::string instance_hash = request->header().instance_hash();

  grpc::Status status = grpc::Status::OK;
  if (!(status = CheckIdValid(id)).ok()) {
    return status;
  }

  SqrtOramServerStorage* const storage =
      dynamic_cast<SqrtOramServerStorage*>(storages_[id].get());

  if (storage == nullptr ||
      OramStorageType::kSqrtStorage != storage->GetOramStorageType()) {
    return grpc::Status(grpc::StatusCode::UNAVAILABLE, oram_type_mismatch_err);
  } else if (storage->GetInstanceHash() != instance_hash) {
    return grpc::Status(grpc::StatusCode::UNAVAILABLE, oram_hash_mismatch_err);
  }

  // Explanation:
  // 0 => shelter;
  // 1 => main memory;
  // 2 => dummy.
  const uint32_t read_type = request->read_from();
  const uint32_t tag = request->tag();
  if (!storage->Check(tag, read_type)) {
    return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT,
                        "Sanity check failed.");
  }

  switch (read_type) {
    case 0: {
      response->set_content(storage->ReadBlockFromShelter(tag));
      break;
    }
    case 1: {
      response->set_content(storage->ReadBlockFromMain(tag));
      break;
    }
    case 2: {
      response->set_content(storage->ReadBlockFromDummy(tag));
      break;
    }
    default:
      // Simply does nothing because the sanity check must be passed.
      break;
  }

  return grpc::Status::OK;
}

grpc::Status OramService::WriteSqrtMemory(grpc::ServerContext* context,
                                          const WriteSqrtMessage* request,
                                          google::protobuf::Empty* empty) {
  logger->info("From peer: {}, WriteSqrtMemory request received.",
               context->peer());

  const uint32_t id = request->header().id();
  const std::string instance_hash = request->header().instance_hash();

  grpc::Status status = grpc::Status::OK;
  if (!(status = CheckIdValid(id)).ok()) {
    return status;
  }

  SqrtOramServerStorage* const storage =
      dynamic_cast<SqrtOramServerStorage* const>(storages_[id].get());
  if (storage == nullptr ||
      storage->GetOramStorageType() != OramStorageType::kFlatStorage) {
    return grpc::Status(grpc::StatusCode::UNAVAILABLE, oram_type_mismatch_err);
  } else if (storage->GetInstanceHash().compare(instance_hash)) {
    return grpc::Status(grpc::StatusCode::UNAVAILABLE, oram_hash_mismatch_err);
  }

  // Write to the memory.
  bool write_to_cache = request->write_to_cache();
  const uint32_t pos = request->pos();
  const std::string content = request->content();

  // Should always check boundary before writing.
  if (write_to_cache) {
    if (!storage->Check(pos, 0)) {
      return grpc::Status(grpc::StatusCode::OUT_OF_RANGE,
                          "The given position is out of range.");
    }
    storage->WriteBlockToShelter(pos, content);
  } else {
    if (!storage->Check(pos, 1)) {
      return grpc::Status(grpc::StatusCode::OUT_OF_RANGE,
                          "The given position is out of range.");
    }
    storage->WriteBlockToMain(pos, content);
  }

  return grpc::Status::OK;
}

grpc::Status OramService::SqrtPermute(grpc::ServerContext* context,
                                      const SqrtPermMessage* request,
                                      google::protobuf::Empty* empty) {
  logger->info("From peer: {}, SqrtPermute request received.", context->peer());

  const uint32_t id = request->header().id();
  const std::string instance_hash = request->header().instance_hash();

  grpc::Status status = grpc::Status::OK;
  if (!(status = CheckIdValid(id)).ok()) {
    return status;
  }

  SqrtOramServerStorage* const storage =
      dynamic_cast<SqrtOramServerStorage* const>(storages_[id].get());
  if (storage == nullptr ||
      storage->GetOramStorageType() != OramStorageType::kFlatStorage) {
    return grpc::Status(grpc::StatusCode::UNAVAILABLE, oram_type_mismatch_err);
  } else if (storage->GetInstanceHash().compare(instance_hash)) {
    return grpc::Status(grpc::StatusCode::UNAVAILABLE, oram_hash_mismatch_err);
  }

  const std::vector<uint32_t> perm =
      std::move(oram_utils::StrToPerm(request->content()));
  storage->DoPermute(perm);

  return grpc::Status::OK;
}

grpc::Status OramService::ReadFlatMemory(grpc::ServerContext* context,
                                         const ReadFlatRequest* request,
                                         FlatVectorMessage* response) {
  logger->info("From peer: {}, ReadFlagMemory request received.",
               context->peer());

  const uint32_t id = request->header().id();
  const std::string instance_hash = request->header().instance_hash();

  grpc::Status status = grpc::Status::OK;
  if (!(status = CheckIdValid(id)).ok()) {
    return status;
  }

  FlatOramServerStorage* const storage =
      dynamic_cast<FlatOramServerStorage* const>(storages_[id].get());
  if (storage == nullptr ||
      storage->GetOramStorageType() != OramStorageType::kFlatStorage) {
    return grpc::Status(grpc::StatusCode::UNAVAILABLE, oram_type_mismatch_err);
  } else if (storage->GetInstanceHash().compare(instance_hash)) {
    return grpc::Status(grpc::StatusCode::UNAVAILABLE, oram_hash_mismatch_err);
  }

  const server_flat_storage_t blocks = storage->GetStorage();
  response->set_content(blocks);

  return status;
}

grpc::Status OramService::WriteFlatMemory(grpc::ServerContext* context,
                                          const FlatVectorMessage* request,
                                          google::protobuf::Empty* empty) {
  logger->info("From peer: {}, WriteFlatMemory request received.",
               context->peer());

  const uint32_t id = request->header().id();
  const std::string instance_hash = request->header().instance_hash();

  grpc::Status status = grpc::Status::OK;
  if (!(status = CheckIdValid(id)).ok()) {
    return status;
  }

  FlatOramServerStorage* const storage =
      dynamic_cast<FlatOramServerStorage* const>(storages_[id].get());
  if (storage == nullptr ||
      storage->GetOramStorageType() != OramStorageType::kFlatStorage) {
    return grpc::Status(grpc::StatusCode::UNAVAILABLE, oram_type_mismatch_err);
  } else if (storage->GetInstanceHash().compare(instance_hash)) {
    return grpc::Status(grpc::StatusCode::UNAVAILABLE, oram_hash_mismatch_err);
  }

  storage->ResetStorage();
  storage->From(request->content());

  return status;
}

grpc::Status OramService::ResetServer(grpc::ServerContext* context,
                                      const google::protobuf::Empty* request,
                                      google::protobuf::Empty* response) {
  logger->info("From peer: {}, Reset server.", context->peer());

  storages_.clear();
  cryptor_.reset();

  return grpc::Status::OK;
}

grpc::Status OramService::PrintOramTree(grpc::ServerContext* context,
                                        const PrintOramTreeRequest* request,
                                        google::protobuf::Empty* response) {
  logger->info("From peer: {}, PrintOramTree request received.",
               context->peer());

  const uint32_t id = request->id();

  grpc::Status status = grpc::Status::OK;
  if (!(status = CheckIdValid(id)).ok()) {
    return status;
  }

  // Check if the storage is tree ORAM.
  TreeOramServerStorage* const storage =
      dynamic_cast<TreeOramServerStorage* const>(storages_[id].get());
  if (storage == nullptr ||
      storage->GetOramStorageType() != OramStorageType::kTreeStorage) {
    return grpc::Status(grpc::StatusCode::UNAVAILABLE, oram_type_mismatch_err);
  }

  oram_utils::PrintOramTree(std::move(storage->GetStorage()));

  return status;
}

grpc::Status OramService::ReadPath(grpc::ServerContext* context,
                                   const ReadPathRequest* request,
                                   ReadPathResponse* response) {
  logger->info("From peer: {}, ReadPath request received.", context->peer());

  const uint32_t id = request->header().id();
  const std::string instance_hash = request->header().instance_hash();
  const uint32_t path = request->path();
  const uint32_t level = request->level();

  logger->info("PathORAM id: {}, path: {}, level: {}", id, path, level);

  grpc::Status status = grpc::Status::OK;
  if (!(status = CheckIdValid(id)).ok()) {
    return status;
  }

  // Check if the storage is tree ORAM.
  TreeOramServerStorage* const storage =
      dynamic_cast<TreeOramServerStorage* const>(storages_[id].get());
  if (storage == nullptr ||
      storage->GetOramStorageType() != OramStorageType::kTreeStorage) {
    return grpc::Status(grpc::StatusCode::UNAVAILABLE, oram_type_mismatch_err);
  } else if (storage->GetInstanceHash().compare(instance_hash)) {
    return grpc::Status(grpc::StatusCode::UNAVAILABLE, oram_hash_mismatch_err);
  }

  // Read the path and record the time it used.
  auto begin = std::chrono::high_resolution_clock::now();

  p_oram_bucket_t bucket;
  if (!storage->ReadPath(level, path, &bucket).ok()) {
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

  return status;
}

grpc::Status OramService::WritePath(grpc::ServerContext* context,
                                    const WritePathRequest* request,
                                    WritePathResponse* response) {
  logger->info("From peer: {}, WritePath request received.", context->peer());
  Type type = request->type();

  const uint32_t id = request->header().id();
  const std::string instance_hash = request->header().instance_hash();
  const uint32_t level = request->level();
  const uint32_t path = request->path();
  const uint32_t offset = request->offset();

  grpc::Status server_status = grpc::Status::OK;
  if (!(server_status = CheckIdValid(id)).ok()) {
    return server_status;
  }

  // Check if the storage is tree ORAM.
  TreeOramServerStorage* const storage =
      dynamic_cast<TreeOramServerStorage* const>(storages_[id].get());
  if (storage == nullptr ||
      storage->GetOramStorageType() != OramStorageType::kTreeStorage) {
    return grpc::Status(grpc::StatusCode::UNAVAILABLE, oram_type_mismatch_err);
  } else if (storage->GetInstanceHash().compare(instance_hash)) {
    return grpc::Status(grpc::StatusCode::UNAVAILABLE, oram_hash_mismatch_err);
  }

  // Deserialize the bucket from the string.
  p_oram_bucket_t bucket = std::move(
      oram_utils::DeserializeFromStringVector(std::vector<std::string>(
          request->bucket().begin(), request->bucket().end())));

  logger->debug("After deserialize:");
  oram_utils::PrintStash(bucket);

  // Write the path.
  OramStatus status =
      type == Type::kInit
          ? storage->AccurateWritePath(level, offset, bucket, type)
          : storage->WritePath(level, path, bucket);

  if (!status.ok()) {
    const std::string error_message =
        oram_utils::StrCat("Failed to write path: ", path, " in level: ", level,
                           " in PathORAM id: ", id);
    return grpc::Status(grpc::StatusCode::INTERNAL, error_message);
  }

  return server_status;
}

grpc::Status OramService::KeyExchange(grpc::ServerContext* context,
                                      const KeyExchangeRequest* request,
                                      KeyExchangeResponse* response) {
  const std::string public_key_client = request->public_key_client();

  OramStatus status;
  if (!(status = cryptor_->SampleKeyPair()).ok()) {
    const std::string error_message = oram_utils::StrCat(
        "Failed to sample key pair! Error: ", status.ErrorMessage());
    return grpc::Status(grpc::StatusCode::INTERNAL, error_message);
  }

  if (!(status = cryptor_->SampleSessionKey(public_key_client, 1)).ok()) {
    const std::string error_message = oram_utils::StrCat(
        "Failed to sample session key! Error: ", status.ErrorMessage());
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

grpc::Status OramService::CloseConnection(
    grpc::ServerContext* context, const google::protobuf::Empty* request,
    google::protobuf::Empty* response) {
  logger->info("Closing connection...");
  server_running = false;
  return grpc::Status::OK;
}

grpc::Status OramService::SendHello(grpc::ServerContext* context,
                                    const HelloMessage* request,
                                    google::protobuf::Empty* empty) {
  const std::string encrypted_message = request->content();
  const std::string iv = request->iv();
  std::string message;
  OramStatus status;

  logger->info("Received encrypted message: {}.",
               spdlog::to_hex(encrypted_message));

  if (!(status = cryptor_->Decrypt((uint8_t*)encrypted_message.data(),
                                   encrypted_message.size(),
                                   (uint8_t*)iv.data(), &message))
           .ok()) {
    const std::string error_message = oram_utils::StrCat(
        "Failed to verify Hello message! Error: ", status.ErrorMessage());
    return grpc::Status(grpc::StatusCode::INTERNAL, error_message);
  }

  logger->info("Successfully verified: {}.", message);
  return grpc::Status::OK;
}

grpc::Status OramService::ReportServerInformation(
    grpc::ServerContext* context, const google::protobuf::Empty* request,
    google::protobuf::Empty* response) {
  logger->info("Report server information...");

  double storage_size = 0;
  for (const auto& storage : storages_) {
    storage_size += storage.second->ReportStorage();
  }

  logger->info("The total storage size is {} MB.", storage_size);

  return grpc::Status::OK;
}

ServerRunner::ServerRunner(const std::string& address, uint32_t port,
                           const std::string& key_path,
                           const std::string& crt_path)
    : address_(address), port_(port) {
  const std::string key_file = oram_utils::ReadKeyCrtFile(key_path);
  const std::string crt_file = oram_utils::ReadKeyCrtFile(crt_path);

  if (key_file.empty() || crt_file.empty()) {
    logger->error("[-] Neither the certificate nor the key cannot be empty.");

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

  service_ = std::make_unique<OramService>();
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
}  // namespace oram_impl