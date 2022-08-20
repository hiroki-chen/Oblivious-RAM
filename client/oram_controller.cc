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
#include "oram_controller.h"

#include <algorithm>
#include <cmath>
#include <chrono>

#include <spdlog/spdlog.h>
#include <spdlog/fmt/bin_to_hex.h>

#include "base/oram_crypto.h"
#include "base/oram_utils.h"

extern std::shared_ptr<spdlog::logger> logger;

using std::chrono_literals::operator""us;

namespace partition_oram {
// The ownership of ORAM main controller cannot be multiple.
// This cannot be shared.
std::unique_ptr<OramController> OramController::GetInstance() {
  static std::unique_ptr<OramController> instance(new OramController());
  return std::move(instance);
}

size_t OramController::counter_ = 0;

PathOramController::PathOramController(uint32_t id, uint32_t block_num,
                                       uint32_t bucket_size)
    : id_(id),
      bucket_size_(bucket_size),
      network_time_(0us),
      network_communication_(0ul) {
  const size_t bucket_num = std::ceil(block_num * 1.0 / bucket_size);
  // Note that the level starts from 0.
  tree_level_ = std::ceil(LOG_BASE(bucket_num + 1, 2)) - 1;
  number_of_leafs_ = POW2(tree_level_);

  logger->debug(
      "PathORAM Config:\n"
      "id: {}, number_of_leafs: {}, bucket_size: {}, tree_height: {}\n",
      id_, number_of_leafs_, bucket_size_, tree_level_);
  cryptor_ = oram_crypto::Cryptor::GetInstance();
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

Status PathOramController::PrintOramTree(void) {
  grpc::ClientContext context;
  PrintOramTreeRequest request;
  google::protobuf::Empty empty;

  request.set_id(id_);
  grpc::Status status = stub_->PrintOramTree(&context, request, &empty);

  if (!status.ok()) {
    logger->error("PrintOramTree failed: {}", status.error_message());
    return Status::kServerError;
  }

  return Status::kOK;
}

Status PathOramController::AccurateWriteBucket(uint32_t level, uint32_t offset,
                                               const p_oram_bucket_t& bucket) {
  grpc::ClientContext context;
  WritePathRequest request;
  WritePathResponse response;

  request.set_id(id_);
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
    return Status::kServerError;
  }

  return Status::kOK;
}

Status PathOramController::InitOram(void) {
  grpc::ClientContext context;
  InitOramRequest request;
  google::protobuf::Empty empty;

  request.set_id(id_);
  request.set_bucket_size(bucket_size_);
  request.set_bucket_num(number_of_leafs_);

  grpc::Status status = stub_->InitOram(&context, request, &empty);
  if (!status.ok()) {
    logger->error("InitOram failed: {}", status.error_message());
    return Status::kServerError;
  }

  return Status::kOK;
}

// The input vector of blocks should be sorted by their path id.
Status PathOramController::FillWithData(const std::vector<oram_block_t>& data) {
  // We organize all the data into buckets and then directly write them to the
  // server by invoking the WritePath method provided by the gRPC framework.
  // The data are organized level by level, and for best performance, we
  // initialize the ORAM tree from the leaf to the root. In other words, we
  // **GREEDILY** fill the buckets from the leaf to the root.

  logger->debug("Fill With Data to Path ORAM id {}", id_);
  oram_utils::PrintStash(data);
  logger->debug("---------------------------------");

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
          oram_utils::CheckStatus(
              oram_crypto::Cryptor::UniformRandom(begin, end, &path),
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

  // Print the oram tree on the server side.
  grpc::ClientContext context;
  PrintOramTreeRequest request;
  google::protobuf::Empty empty;

  request.set_id(id_);

  return Status::kOK;
}

Status PathOramController::ReadBucket(uint32_t path, uint32_t level,
                                      p_oram_bucket_t* const bucket) {
  if (path >= number_of_leafs_ || level > tree_level_) {
    return Status::kInvalidArgument;
  }

  grpc::ClientContext context;

  // Then prepare for RPC call.
  ReadPathRequest request;
  ReadPathResponse response;
  request.set_id(id_);
  request.set_path(path);
  request.set_level(level);

  auto begin = std::chrono::high_resolution_clock::now();
  grpc::Status status = stub_->ReadPath(&context, request, &response);
  auto end = std::chrono::high_resolution_clock::now();

  network_time_ +=
      std::chrono::duration_cast<std::chrono::microseconds>(end - begin);

  if (!status.ok()) {
    return Status::kServerError;
  }

  const size_t bucket_size = response.bucket_size();
  // Then copy the bucket to the vector.
  for (size_t j = 0; j < bucket_size; j++) {
    oram_block_t* const block = (oram_block_t*)malloc(ORAM_BLOCK_SIZE);
    oram_utils::ConvertToBlock(response.bucket(j), block);

    // Decrypt the block.
    oram_utils::DecryptBlock(block, cryptor_.get());

    bucket->emplace_back(*block);
    oram_utils::SafeFree(block);
  }

  network_communication_ += response.bucket_size();

  return Status::kOK;
}

Status PathOramController::WriteBucket(uint32_t path, uint32_t level,
                                       const p_oram_bucket_t& bucket) {
  grpc::ClientContext context;
  WritePathRequest request;
  WritePathResponse response;

  request.set_id(id_);
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
    return Status::kServerError;
  }

  return Status::kOK;
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
Status PathOramController::Access(Operation op_type, uint32_t address,
                                  oram_block_t* const data, bool dummy) {
  logger->debug("ORAM ID: {}, Accessing address {}, op_type {}, dummy {} ", id_,
                address, (int)op_type, dummy);
  // First we do a sanity check.
  PANIC_IF(op_type == Operation::kInvalid, "Invalid operation.");

  // Next, we shall the get the real path of the current block.
  // @ref Stefanov's paper for full details.
  // Steps: 1-2
  // Randomly remap the position of block a to a new random position.
  // Let x denote the block’s old position.
  uint32_t x;
  // We sample a random path in advance to handle the case when dummy = true.
  oram_utils::CheckStatus(
      oram_crypto::Cryptor::UniformRandom(0, number_of_leafs_ - 1, &x),
      "UniformRandom error");

  if (!dummy) {
    if (position_map_.find(address) == position_map_.end()) {
      position_map_[address] = x;
      // Insert the block to the stash.
      stash_.emplace_back(*data);
    } else {
      uint32_t prev = position_map_[address];
      // Use x as the block's path.
      position_map_[address] = x;
      x = prev;
    }
  }

  // Step 3-5: Read the whole path from the server into the stash.
  p_oram_path_t bucket_this_path;

  for (size_t i = 0; i <= tree_level_; i++) {
    p_oram_bucket_t bucket_this_level;
    Status status = ReadBucket(x, i, &bucket_this_level);
    oram_utils::CheckStatus(status,
                            oram_utils::StrCat("Failed to read bucket: ", x));
    bucket_this_path.emplace_back(bucket_this_level);
  }

  if (dummy) {
    // Invoke a dummy read operation and everything is done here.
    return Status::kOK;
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
  logger->debug("------------------------------------------------------");
  oram_utils::PrintStash(stash_);
  logger->debug("------------------------------------------------------");
  PANIC_IF(iter == stash_.end(), "Failed to find the block in the stash.");

  // Update the block.
  if (op_type == Operation::kWrite) {
    memcpy(&(*iter), data, ORAM_BLOCK_SIZE);
  } else {
    memcpy(data, &(*iter), ORAM_BLOCK_SIZE);
    // For Partition ORAM. => READ AND REMOVE.
    stash_.erase(iter);
    position_map_.erase(address);
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
    Status status = WriteBucket(x, i - 1, subset);
    oram_utils::CheckStatus(status, "Failed to write bucket.");
  }

  return Status::kOK;
}

// @ ref Towards Practical Oblivious RAM's access algorithm.
// Algorithm for data access. Read or write a data block identified by u.
// - If op = read, the input parameter data* = None, and the Access operation
// returns the newly fetched block.
// - If op = write, the Access operation writes the specified data* to the block
// identified by u, and returns the old value of the block u.
//
// For readability, we rename u to address denoting the block's identifier.
Status OramController::Access(Operation op_type, uint32_t address,
                              oram_block_t* const data) {
  auto begin_access = std::chrono::high_resolution_clock::now();
  // A temporary buffer that holds the data.
  oram_block_t block;
  // Sample a new random slot id for this block.
  uint32_t new_slot_id;
  Status status = oram_crypto::Cryptor::UniformRandom(
      0, path_oram_controllers_.size() - 1, &new_slot_id);
  oram_utils::CheckStatus(status, "Failed to sample a new slot id.");

  // Get the position (i.e., the slot id) from the position map.
  const uint32_t slot_id = position_map_[address];
  logger->debug("SLOT ID {} FOR ADDRESS {}", slot_id, address);
  // Then immediately update the position map.
  position_map_[address] = new_slot_id;

  logger->debug("New slot id: {} for address: {}", new_slot_id, address);

  // Get the PathOram controller.
  PathOramController* const controller = path_oram_controllers_[slot_id].get();
  // Check if the block is already in the slot.
  // If there is no such block, we read it from the server and then
  // add it to the slot.
  auto iter = std::find_if(slots_[slot_id].begin(), slots_[slot_id].end(),
                           BlockEqual(address));
  if (iter == slots_[slot_id].end()) {
    // Read the block from the PathORAM controller.
    status = controller->Access(op_type, address, &block, false);
  } else {
    // Invoke a dummy read.
    Status status =
        controller->Access(Operation::kRead, address, nullptr, true);
    oram_utils::CheckStatus(status, "Cannot perform fake read!");
    // Read the block directly from the slot.
    block = *iter;
    // Expire the iterator.
    slots_[slot_id].erase(iter);
  }

  // Update the block if the operation is write.
  if (op_type == Operation::kWrite) {
    block = *data;
  } else {
    *data = block;
  }

  // Add the block to the slot.
  slots_[new_slot_id].emplace_back(block);

  auto end_access = std::chrono::high_resolution_clock::now();

  logger->info("[+] Access time: {} us.",
               std::chrono::duration_cast<std::chrono::microseconds>(
                   end_access - begin_access)
                   .count());

  // Call piggy-backed eviction. (optional)
  // NO piggyback-ed eviction is implemented for PathORAM.

  // status = SequentialEvict();
  auto begin_evict = std::chrono::high_resolution_clock::now();
  status = RandomEvict();
  oram_utils::CheckStatus(status, "Failed to perform eviction!");
  auto end_evict = std::chrono::high_resolution_clock::now();

  logger->info("[+] Eviction time: {} us.",
               std::chrono::duration_cast<std::chrono::microseconds>(
                   end_evict - begin_evict)
                   .count());

  return Status::kOK;
}

Status OramController::Evict(uint32_t id) {
  logger->debug("Evicting slot {}", id);
  PathOramController* const controller = path_oram_controllers_[id].get();
  if (slots_[id].empty()) {
    // Perform a fake write.
    return controller->Access(Operation::kWrite, 0, nullptr, true);
  } else {
    logger->debug("---------------EVICT------------------");
    oram_utils::PrintStash(slots_[id]);
    logger->debug("---------------EVICT------------------");
    oram_block_t block = slots_[id].back();
    slots_[id].pop_back();
    return controller->Access(Operation::kWrite, block.header.block_id, &block,
                              false);
  }
}

// RandomEvict samples \nu \in \mathbb{N} random slots (with replacement) to
// evict from.
Status OramController::RandomEvict(void) {
  // For simplicity, we use uniform random sampling.
  for (size_t i = 0; i < nu_; i++) {
    uint32_t id;
    oram_utils::CheckStatus(oram_crypto::Cryptor::UniformRandom(
                                0, path_oram_controllers_.size() - 1, &id),
                            "Failed to sample a new slot id.");
    if (Evict(id) != Status::kOK) {
      return Status::kInvalidOperation;
    }
  }

  return Status::kOK;
}

// SequentialEvict determines the number of blocks to evict num based on a
// prescribed dis- tribution D(ν) and sequentially scans num slots to evict
// from. RandomEvict samples ν ∈ N random slots (with replacement) to evict
// from.
Status OramController::SequentialEvict(void) {
  size_t evict_num;
  oram_utils::CheckStatus(
      oram_crypto::Cryptor::UniformRandom(0, path_oram_controllers_.size() - 1,
                                          (uint32_t*)&evict_num),
      "Failed to sample eviction number.");
  for (size_t i = 0; i < evict_num; i++) {
    // cnt is a global counter for the sequential scan.
    counter_ = (counter_ + 1) % partition_size_;
    if (Evict(counter_) != Status::kOK) {
      return Status::kInvalidOperation;
    }
  }

  return Status::kOK;
}

Status OramController::Run(uint32_t block_num, uint32_t bucket_size) {
  logger->info("[+] The Partition Oram Controller is running...");
  // Determine the size of each sub-ORAM and the number of slot number.
  const size_t squared = std::ceil(std::sqrt(block_num));
  partition_size_ = std::ceil(squared * (1));
  block_num_ = block_num;

  logger->debug("The Partition ORAM's config: partition_size = {} ",
                partition_size_);
  // Initialize all the slots.
  slots_.resize(squared);

  for (size_t i = 0; i < squared; i++) {
    // We create the PathORAM controller for each slot.
    path_oram_controllers_.emplace_back(
        std::make_unique<PathOramController>(i, partition_size_, bucket_size));
    path_oram_controllers_.back()->SetStub(stub_);

    // Then invoke the intialization procedure.
    Status status = path_oram_controllers_.back()->InitOram();
    if (status != Status::kOK) {
      return status;
    }
  }

  return Status::kOK;
}

Status OramController::ProcessSlot(const std::vector<oram_block_t>& data,
                                   uint32_t slot_id) {
  // Initialize the position map.
  std::for_each(data.begin(), data.end(), [&](const oram_block_t& block) {
    if (block.header.type == BlockType::kNormal) {
      position_map_[block.header.block_id] = slot_id;
    }
  });

  return Status::kOK;
}

Status OramController::FillWithData(const std::vector<oram_block_t>& data) {
  const size_t level = path_oram_controllers_.front()->GetTreeLevel();
  const size_t tree_size = (POW2(level + 1) - 1) * bucket_size_;
  // Check if the data size is consistent with the block number (note that this
  // includes dummy blocks).
  if (data.size() != tree_size * path_oram_controllers_.size()) {
    return Status::kInvalidArgument;
  }

  // Send the data vector to each PathORAM controller.
  auto begin = std::chrono::high_resolution_clock::now();
  for (size_t i = 0; i < path_oram_controllers_.size(); i++) {
    // Slice the data vector.
    const std::vector<oram_block_t> cur_data(
        data.begin() + i * tree_size, data.begin() + (i + 1) * tree_size);
    Status status = ProcessSlot(cur_data, i);
    oram_utils::CheckStatus(status, "Failed to process slot!");

    // Initialize the Path Oram.
    status = path_oram_controllers_[i]->FillWithData(cur_data);
    oram_utils::CheckStatus(status,
                            "Failed to fill the data into the Path ORAM.");
  }
  auto end = std::chrono::high_resolution_clock::now();

  logger->info(
      "[+] The Partition Oram Controller is initialized. Elapsed time is {} "
      "us.",
      std::chrono::duration_cast<std::chrono::microseconds>(end - begin)
          .count());

  return Status::kOK;
}

Status OramController::TestPathOram(uint32_t controller_id) {
  if (controller_id >= path_oram_controllers_.size()) {
    logger->error("The controller id is out of range.");
    return Status::kOutOfRange;
  }

  PathOramController* const controller =
      path_oram_controllers_[controller_id].get();

  const size_t level = controller->GetTreeLevel();
  const size_t tree_size = (POW2(level + 1) - 1) * bucket_size_;
  const p_oram_bucket_t raw_data = std::move(
      oram_utils::SampleRandomBucket(partition_size_, tree_size, 0ul));

  controller->FillWithData(raw_data);

  logger->info("[+] Begin testing Path ORAM...");
  auto begin = std::chrono::high_resolution_clock::now();

  for (size_t i = 0; i < partition_size_; i++) {
    oram_block_t block;
    Status status = controller->Access(Operation::kRead, i, &block, false);
    oram_utils::CheckStatus(status, "Failed to read block.");

    logger->debug("[+] Read block {}: {}", block.header.block_id,
                  block.data[0]);
  }

  for (size_t i = 0; i < partition_size_; i++) {
    oram_block_t block;
    block.header.block_id = i;
    block.header.type = BlockType::kNormal;
    block.data[0] = partition_size_ - i;
    Status status = controller->Access(Operation::kWrite, i, &block, false);
    oram_utils::CheckStatus(status, "Failed to write block.");
  }

  for (size_t i = 0; i < partition_size_; i++) {
    oram_block_t block;
    Status status = controller->Access(Operation::kRead, i, &block, false);
    oram_utils::CheckStatus(status, "Failed to read block.");

    logger->debug("[+] Read block {}: {}", block.header.block_id,
                  block.data[0]);
  }

  auto end = std::chrono::high_resolution_clock::now();
  logger->info(
      "[-] End Testing Path ORAM. Time elapsed: {} ms.",
      std::chrono::duration_cast<std::chrono::milliseconds>(end - begin)
          .count());

  return Status::kOK;
}

Status OramController::TestPartitionOram(void) {
  std::vector<oram_block_t> blocks;
  const size_t level = path_oram_controllers_.front()->GetTreeLevel();
  const size_t tree_size = (POW2(level + 1) - 1) * bucket_size_;

  // Sample random data.
  for (size_t i = 0; i < path_oram_controllers_.size(); i++) {
    const std::vector<oram_block_t> block =
        std::move(oram_utils::SampleRandomBucket(partition_size_, tree_size,
                                                 i * partition_size_ / 2));
    logger->debug("[+] Sample for {}: {}", i, block.size());
    oram_utils::PrintStash(block);
    blocks.insert(blocks.end(), block.begin(), block.end());
  }

  // Initialize the Partition ORAM.
  Status status = FillWithData(blocks);
  oram_utils::CheckStatus(status,
                          "Failed to fill the data into the Partition ORAM.");

  // Report the information.
  grpc::ClientContext context;
  google::protobuf::Empty empty;
  stub_->ReportServerInformation(&context, empty, &empty);

  auto begin = std::chrono::high_resolution_clock::now();
  logger->info("[+] Begin testing Partition ORAM...");
  for (size_t i = 0; i < 10; i++) {
    oram_block_t block;
    logger->debug("[+] Reading {} ...", i);
    Status status = Access(Operation::kRead, i, &block);
    oram_utils::CheckStatus(status, "Cannot access partition ORAM!");

    PANIC_IF((block.data[0] != i), "Failed to read the correct block.");
    logger->debug("[+] Read block {}: {}", block.header.block_id,
                  block.data[0]);
  }
  auto end = std::chrono::high_resolution_clock::now();

  // Report the storage.
  const double storage = ReportClientStorage();

  // Report the communication.
  const double communication = ReportNetworkCommunication();

  auto end_to_end =
      std::chrono::duration_cast<std::chrono::microseconds>(end - begin);
  const auto network_time = ReportNetworkingTime();
  const auto client_time = end_to_end - network_time;

  logger->info("[-] The client storage is {} MB.", storage / 1024 / 1024);
  logger->info("[-] The client communication is {} MB.",
               communication / 1024 / 1024 / 10);
  logger->info(
      "[-] End testing Partition ORAM.\nEnd-to-end time elapsed per block: {} "
      "us. \nClient computation time is: {} us.",
      (end_to_end / 10).count(), (client_time / 10).count());

  return Status::kOK;
}

size_t OramController::ReportClientStorage(void) const {
  size_t client_storage = 0;

  std::for_each(
      path_oram_controllers_.begin(), path_oram_controllers_.end(),
      [&client_storage](
          const std::unique_ptr<PathOramController>& path_oram_controller) {
        client_storage += path_oram_controller->ReportClientStorage();
      });

  client_storage += slots_.size() * ORAM_BLOCK_SIZE;
  return client_storage;
}

std::chrono::microseconds OramController::ReportNetworkingTime(void) const {
  std::chrono::microseconds ans = 0us;

  for (const auto& controller : path_oram_controllers_) {
    ans += controller->ReportNetworkingTime();
  }

  return ans;
}

size_t OramController::ReportNetworkCommunication(void) const {
  size_t ans = 0;

  for (const auto& controller : path_oram_controllers_) {
    ans += controller->ReportNetworkCommunication();
  }

  return ans;
}

}  // namespace partition_oram
