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
#include "linear_oram_controller.h"

#include <spdlog/fmt/bin_to_hex.h>
#include <spdlog/spdlog.h>

#include "oram.h"

extern std::shared_ptr<spdlog::logger> logger;

namespace oram_impl {
OramStatus LinearOramController::ReadFromServer(std::string* const out) {
  grpc::ClientContext context;
  ReadFlatRequest request;
  FlatVectorMessage response;

  ASSEMBLE_HEADER(request, id_, instance_hash_, GetVersion());

  // Read the whole storage from the remote storage.
  grpc::Status status = stub_->ReadFlatMemory(&context, request, &response);

  if (status.ok()) {
    *out = response.content();

    return OramStatus::OK;
  }

  return OramStatus(StatusCode::kServerError, status.error_message(), __func__);
}

OramStatus LinearOramController::WriteToServer(const std::string& input) {
  grpc::ClientContext context;
  FlatVectorMessage request;
  google::protobuf::Empty empty;

  ASSEMBLE_HEADER(request, id_, instance_hash_, GetVersion());
  request.set_content(input);

  grpc::Status status = stub_->WriteFlatMemory(&context, request, &empty);

  if (!status.ok()) {
    return OramStatus(StatusCode::kServerError, status.error_message(),
                      __func__);
  }

  return OramStatus::OK;
}

OramStatus LinearOramController::InternalAccess(Operation op_type,
                                                uint32_t address,
                                                oram_block_t* const data,
                                                bool dummy) {
  PANIC_IF(op_type == Operation::kInvalid, "Invalid ORAM operation!");

  // For every R/W operation:
  //    – Client reads entire storage, item by item.
  //    – Re-encrypts each item after possibly changing it.
  //    – Writes the item back to remote storage.
  // O(n) overhead per each R/W operation.
  std::string storage;
  OramStatus status = OramStatus::OK;
  if (!(status = ReadFromServer(&storage)).ok()) {
    return status.Append(OramStatus(StatusCode::kInvalidOperation,
                                    "Cannot read from the server", __func__));
  }

  // Reinterpret the storage as an array of oram_block_t.
  oram_block_t* block_ptr = reinterpret_cast<oram_block_t*>(storage.data());
  for (size_t i = 0; i < (storage.size() / ORAM_BLOCK_SIZE); i++) {
    // Decrypt the block.
    oram_utils::DecryptBlock(block_ptr, cryptor_.get());

    if (block_ptr->header.type == BlockType::kNormal &&
        block_ptr->header.block_id == address) {
      // Read or write.
      if (op_type == Operation::kRead) {
        memcpy(data, block_ptr, ORAM_BLOCK_SIZE);
      } else {
        memcpy(block_ptr->data, data->data, DEFAULT_ORAM_DATA_SIZE);
      }
    }

    // Re-encrypt the block whenever this is the target block.
    oram_utils::EncryptBlock(block_ptr, cryptor_.get());

    // Increment the pointer.
    block_ptr += 1;
  }

  return WriteToServer(storage).ok()
             ? OramStatus::OK
             : OramStatus(StatusCode::kServerError,
                          "Cannot invoke WriteToServer on the server side",
                          __func__);
}

OramStatus LinearOramController::InitOram(void) {
  grpc::ClientContext context;
  InitFlatOramRequest request;
  google::protobuf::Empty empty;

  ASSEMBLE_HEADER(request, id_, instance_hash_, GetVersion());
  request.set_capacity(block_num_);
  request.set_block_size(ORAM_BLOCK_SIZE);

  return stub_->InitFlatOram(&context, request, &empty).ok()
             ? OramStatus::OK
             : OramStatus(StatusCode::kServerError,
                          "Cannot invoke InitOram on the server side",
                          __func__);
}

OramStatus LinearOramController::FillWithData(
    const std::vector<oram_block_t>& data) {
  // We always assume that `data` is properly permuted in a secure way.
  grpc::ClientContext context;
  FlatVectorMessage request;
  google::protobuf::Empty empty;

  ASSEMBLE_HEADER(request, id_, instance_hash_, GetVersion());

  std::string content;
  for (size_t i = 0; i < data.size(); i++) {
    oram_block_t block = data[i];
    oram_utils::EncryptBlock(&block, cryptor_.get());
    content.append(
        std::string(reinterpret_cast<char*>(&block), ORAM_BLOCK_SIZE));
  }
  request.set_content(content);

  // Set initialized.
  is_initialized_ = true;

  return stub_->WriteFlatMemory(&context, request, &empty).ok()
             ? OramStatus::OK
             : OramStatus(StatusCode::kServerError,
                          "Cannot invoke WriteFlatMemory on the server side",
                          __func__);
}
}  // namespace oram_impl