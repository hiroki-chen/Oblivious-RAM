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
#include "path_oram_controller.h"

#include <spdlog/fmt/bin_to_hex.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <chrono>
#include <cmath>

#include "oram.h"
#include "base/oram_crypto.h"
#include "base/oram_utils.h"

extern std::shared_ptr<spdlog::logger> logger;

using std::chrono_literals::operator""us;

namespace oram_impl {

uint32_t PathOramController::RandomPosition(void) {
  uint32_t x;
  // We sample a random path in advance to handle the case when dummy = true.
  oram_utils::CheckStatus(
      oram_crypto::UniformRandom(0, number_of_leafs_ - 1, &x),
      "UniformRandom error");
  return x;
}

PathOramController::PathOramController(uint32_t id, uint32_t block_num,
                                       uint32_t bucket_size, bool standalone)
    : OramController(id, standalone, block_num, OramType::kPathOram),
      bucket_size_(bucket_size),
      stash_size_(0ul),
      network_time_(0us),
      network_communication_(0ul) {
  const size_t bucket_num = std::ceil(block_num * 1.0 / bucket_size);
  // Note that the level starts from 0.
  tree_level_ = std::ceil(LOG_BASE(bucket_num + 1, 2)) - 1;
  number_of_leafs_ = POW2(tree_level_);

  DBG(logger,
      "PathORAM Config:\n"
      "id: {}, number_of_leafs: {}, bucket_size: {}, tree_height: {}\n",
      id_, number_of_leafs_, bucket_size_, tree_level_);
}

size_t PathOramController::ReportClientStorage(void) const {
  // For the client storage of the Path ORAM, we need to report the number of
  // blocks in the client storage. We exclude the storage of position map for
  // a more straightforward comparison.
  return stash_.size() * ORAM_BLOCK_SIZE;
}

size_t PathOramController::ReportNetworkCommunication(void) const {
  return network_communication_ * ORAM_BLOCK_SIZE;
}

OramStatus PathOramController::PrintOramTree(void) {
  grpc::ClientContext context;
  PrintOramTreeRequest request;
  google::protobuf::Empty empty;

  request.set_id(id_);
  grpc::Status status = stub_->PrintOramTree(&context, request, &empty);

  if (!status.ok()) {
    return OramStatus(StatusCode::kServerError, status.error_message(),
                      __func__);
  }

  return OramStatus::OK;
}

OramStatus PathOramController::AccurateWriteBucket(
    uint32_t level, uint32_t offset, const p_oram_bucket_t& bucket) {
  grpc::ClientContext context;
  WritePathRequest request;
  WritePathResponse response;

  ASSEMBLE_HEADER(request, id_, instance_hash_, GetVersion());
  request.set_level(level);
  request.set_offset(offset);
  request.set_type(Type::kInit);

  // Copy the buckets into the buffer of WriteBucketRequest.
  for (auto block : bucket) {
    // Encrypt the block.
    oram_utils::EncryptBlock(&block, cryptor_.get());
    std::string block_str;
    oram_utils::ConvertToString(&block, &block_str);
    request.add_bucket(block_str);
  }

  grpc::Status status = stub_->WritePath(&context, request, &response);

  if (!status.ok()) {
    return OramStatus(StatusCode::kServerError, status.error_message(),
                      __func__);
  }

  return OramStatus::OK;
}

OramStatus PathOramController::InitOram(void) {
  grpc::ClientContext context;
  InitTreeOramRequest request;
  google::protobuf::Empty empty;

  ASSEMBLE_HEADER(request, id_, instance_hash_, GetVersion());
  request.set_bucket_size(bucket_size_);
  request.set_bucket_num(number_of_leafs_);
  request.set_block_size(ORAM_BLOCK_SIZE);

  grpc::Status status = stub_->InitTreeOram(&context, request, &empty);
  if (!status.ok()) {
    return OramStatus(StatusCode::kServerError, status.error_message(),
                      __func__);
  }

  return OramStatus::OK;
}

OramStatus OramController::FromFile(const std::string& file_path) {
  const std::vector<std::string> data =
      std::move(oram_utils::ReadDataFromFile(file_path));
  // Change the block number.
  block_num_ = data.size();

  p_oram_bucket_t blocks;
  for (size_t i = 0; i < data.size(); i++) {
    oram_block_t block;
    block.header.block_id = i;
    block.header.type = BlockType::kNormal;
    memcpy(block.data, data[i].data(),
           std::min(DEFAULT_ORAM_DATA_SIZE, (int)data[i].size()));
    blocks.emplace_back(block);
  }

  return FillWithData(blocks);
}

// The input vector of blocks should be sorted by their path id.
OramStatus PathOramController::FillWithData(
    const std::vector<oram_block_t>& data) {
  // We organize all the data into buckets and then directly write them to the
  // server by invoking the WritePath method provided by the gRPC framework.
  // The data are organized level by level, and for best performance, we
  // initialize the ORAM tree from the leaf to the root. In other words, we
  // **GREEDILY** fill the buckets from the leaf to the root.
  oram_utils::PrintStash(data);

  size_t p_data = 0;
  for (int i = tree_level_; i >= 0; i--) {
    // We pick bucket_size blocks from the data and organize them into a bucket.
    // The bucket is then sent to the server.
    const uint32_t level_size = POW2(i);
    const uint32_t span = POW2(tree_level_ - i);

    for (uint32_t j = 0; j < level_size; j++) {
      p_oram_bucket_t bucket_this_level;

      // This determined the range of the current bucket in terms of path.
      const uint32_t begin = j * span;
      const uint32_t end = begin + span - 1;

      // Organize into a bucket.
      for (size_t k = 0; k < bucket_size_; k++) {
        if (p_data >= data.size()) {
          break;
        }

        // Sample a random path for the block.
        if (data[p_data].header.type == BlockType::kNormal) {
          uint32_t path;
          oram_utils::CheckStatus(oram_crypto::UniformRandom(begin, end, &path),
                                  "UniformRandom error");
          bucket_this_level.emplace_back(data[p_data]);
          // Update the position map.
          position_map_[data[p_data].header.block_id] = path;
        }

        p_data++;
      }

      oram_utils::PadStash(&bucket_this_level, bucket_size_);

      // Write the bucket to the server.
      oram_utils::CheckStatus(
          AccurateWriteBucket(i, j, bucket_this_level),
          "Failed to write bucket accurately when intializing the ORAM!");
    }
  }

  // Set initialized.
  is_initialized_ = true;

  return OramStatus::OK;
}

OramStatus PathOramController::ReadBucket(uint32_t path, uint32_t level,
                                          p_oram_bucket_t* const bucket) {
  if (path >= number_of_leafs_ || level > tree_level_) {
    return OramStatus(StatusCode::kInvalidArgument,
                      "The path or the level given is not correct", __func__);
  }

  grpc::ClientContext context;

  // Then prepare for RPC call.
  ReadPathRequest request;
  ReadPathResponse response;

  ASSEMBLE_HEADER(request, id_, instance_hash_, GetVersion());
  request.set_path(path);
  request.set_level(level);

  auto begin = std::chrono::high_resolution_clock::now();
  grpc::Status status = stub_->ReadPath(&context, request, &response);
  auto end = std::chrono::high_resolution_clock::now();

  network_time_ +=
      std::chrono::duration_cast<std::chrono::microseconds>(end - begin);

  if (!status.ok()) {
    return OramStatus(StatusCode::kServerError, status.error_message(),
                      __func__);
  }

  const size_t bucket_size = response.bucket_size();
  // Then copy the bucket to the vector.
  for (size_t j = 0; j < bucket_size; j++) {
    oram_block_t block;
    oram_utils::ConvertToBlock(response.bucket(j), &block);

    // Decrypt the block.
    oram_utils::DecryptBlock(&block, cryptor_.get());

    bucket->emplace_back(block);
  }

  network_communication_ += response.bucket_size();

  return OramStatus::OK;
}

OramStatus PathOramController::WriteBucket(uint32_t path, uint32_t level,
                                           const p_oram_bucket_t& bucket) {
  DBG(logger, "[+] Writing bucket at path {}, level {}", path, level);

  grpc::ClientContext context;
  WritePathRequest request;
  WritePathResponse response;

  ASSEMBLE_HEADER(request, id_, instance_hash_, GetVersion());
  request.set_path(path);
  request.set_level(level);
  request.set_type(Type::kNormal);

  // Copy the buckets into the buffer of WriteBucketRequest.
  for (auto block : bucket) {
    // Encrypt the block.
    oram_utils::EncryptBlock(&block, cryptor_.get());

    std::string block_str;
    oram_utils::ConvertToString(&block, &block_str);
    request.add_bucket(block_str);
  }

  network_communication_ += bucket.size();

  auto begin = std::chrono::high_resolution_clock::now();
  grpc::Status status = stub_->WritePath(&context, request, &response);
  auto end = std::chrono::high_resolution_clock::now();

  network_time_ +=
      std::chrono::duration_cast<std::chrono::microseconds>(end - begin);

  if (!status.ok()) {
    return OramStatus(StatusCode::kServerError, status.error_message(),
                      __func__);
  }

  return OramStatus::OK;
}

p_oram_stash_t PathOramController::FindSubsetOf(uint32_t current_path) {
  p_oram_stash_t subset;

  auto iter = stash_.begin();
  while (iter != stash_.end()) {
    const uint32_t block_path = position_map_[iter->header.block_id];

    if (subset.size() < bucket_size_) {
      if (block_path == current_path) {
        subset.emplace_back(*iter);
        // Delete the current block and re-adjust the iterator.
        iter = stash_.erase(iter);
      } else {
        iter++;
      }
    } else {
      break;
    }
  }

  oram_utils::PadStash(&subset, bucket_size_);
  return subset;
}

// If we want to use Path ORAM as the underlying black-box ORAM, we need to
// adapt the following function. This must be very carefully implemented because
// we may need to re-adjust the size of the Path ORAM so that it can hold as
// many blocks as each slot in the Partition ORAM needs.
OramStatus PathOramController::InternalAccess(Operation op_type,
                                              uint32_t address,
                                              oram_block_t* const data,
                                              bool dummy) {
  if (!is_initialized_) {
    return OramStatus(StatusCode::kInvalidOperation,
                      "Cannot access ORAM before it is initialized."
                      " You may need to call `InitOram()` and `FillWithData()` "
                      "method first.",
                      __func__);
  }

  DBG(logger, "ORAM ID: {}, Accessing address {}, op_type {}, dummy {} ", id_,
      address, (int)op_type, dummy);
  // First we do a sanity check.
  PANIC_IF(op_type == Operation::kInvalid, "Invalid operation.");

  // Next, we shall the get the real path of the current block.
  // @ref Stefanov's paper for full details.
  // Steps: 1-2
  // Randomly remap the position of block a to a new random position.
  // Let x denote the block’s old position.
  uint32_t x = RandomPosition();

  if (!dummy) {
    uint32_t prev = position_map_[address];
    // Use x as the block's path.
    position_map_[address] = x;
    x = prev;
  }

  return InternalAccessDirect(op_type, address, x, data, dummy);
}

OramStatus PathOramController::InternalAccessDirect(Operation op_type,
                                                    uint32_t address,
                                                    uint32_t x,
                                                    oram_block_t* const data,
                                                    bool dummy) {
  // Step 3-5: Read the whole path from the server into the stash.
  p_oram_path_t bucket_this_path;

  for (size_t i = 0; i <= tree_level_; i++) {
    p_oram_bucket_t bucket_this_level;
    OramStatus status = ReadBucket(x, i, &bucket_this_level);

    if (!status.ok()) {
      return status.Append(OramStatus(
          StatusCode::kInvalidOperation,
          oram_utils::StrCat("Failed to write bucket ", x), __func__));
    }

    bucket_this_path.emplace_back(bucket_this_level);
  }

  if (dummy) {
    // Invoke a dummy read operation and everything is done here.
    return OramStatus::OK;
  }

  // Read all the blocks into the stash.
  for (size_t i = 0; i <= tree_level_; i++) {
    for (size_t j = 0; j < bucket_this_path[i].size(); j++) {
      oram_block_t block = bucket_this_path[i][j];

      // Check if the block is already in the stash.
      // If there is no such block, we add it to the stash.
      //
      // <=> S = S ∪ ReadBucket(P(x, l))
      auto iter = std::find_if(stash_.begin(), stash_.end(),
                               BlockEqual(block.header.block_id));
      if (iter == stash_.end() && block.header.type == BlockType::kNormal) {
        stash_.emplace_back(block);
      }
    }
  }

  // Step 6-9: Update block, if any.
  // If the access is a write, update the data stored for block a.
  auto iter = std::find_if(stash_.begin(), stash_.end(), BlockEqual(address));
  DBG(logger, "------------------------------------------------------");
  oram_utils::PrintStash(stash_);
  DBG(logger, "------------------------------------------------------");

  if (iter == stash_.end()) {
    return OramStatus(StatusCode::kObjectNotFound,
                      oram_utils::StrCat("Failed to find the block ", address,
                                         " in the stash!"),
                      __func__);
  }

  // HACK: This may be incorrect.
  stash_size_ = std::max(stash_size_, stash_.size());

  // Update the block.
  if (op_type == Operation::kWrite) {
    memcpy(iter->data, data->data, DEFAULT_ORAM_DATA_SIZE);
    // Write the data length as well.
    iter->header.data_len = data->header.data_len;
  } else {
    memcpy(data, &(*iter), ORAM_BLOCK_SIZE);

    if (!standalone_) {
      // For Partition ORAM. => READ AND REMOVE.
      stash_.erase(iter);
      position_map_.erase(address);
    }
  }

  // STEP 10-15: Write the path.
  //
  // Write the path back and possibly include some additional blocks from the
  // stash if they can be placed into the path. Buckets are greedily filled
  // with blocks in the stash in the order of leaf to root, ensuring that
  // blocks get pushed as deep down into the tree as possible. A block a? can
  // be placed in the bucket at level ? only if the path P(position[a']) to
  // the leaf of block a' intersects the path accessed P(x) at level l. In
  // other words, if P(x, l) = P(position[a'], l).

  // Prevent overflow for unsigned variable...
  for (size_t i = tree_level_ + 1; i >= 1; i--) {
    // Find a subset S' of stash such that the element in S' intersects with
    // the current old path of x. I.e., S' ← {(a', data') \in S : P(x, l) =
    // P(position[a'], l)} Select min(|S'|, Z) blocks. If |S'| < Z, then we
    // pad S' with dummy blocks. Expire all blocks in S that are in S'. Write
    // them back.
    p_oram_stash_t subset = std::move(FindSubsetOf(x));
    OramStatus status = WriteBucket(x, i - 1, subset);

    if (!status.ok()) {
      return status.Append(OramStatus(StatusCode::kInvalidOperation,
                                      "Failed to write bucket", __func__));
    }
  }

  return OramStatus::OK;
}

}  // namespace oram_impl