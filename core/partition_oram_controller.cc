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
#include "partition_oram_controller.h"

#include <spdlog/fmt/bin_to_hex.h>
#include <spdlog/spdlog.h>

extern std::shared_ptr<spdlog::logger> logger;

using std::chrono_literals::operator""us;

namespace oram_impl {
size_t PartitionOramController::counter_ = 0;

// The ownership of ORAM main controller cannot be multiple.
// This cannot be shared.
std::unique_ptr<PartitionOramController>
PartitionOramController::GetInstance() {
  static std::unique_ptr<PartitionOramController> instance(
      new PartitionOramController());
  return std::move(instance);
}

// @ ref Towards Practical Oblivious RAM's access algorithm.
// Algorithm for data access. Read or write a data block identified by u.
// - If op = read, the input parameter data* = None, and the Access operation
// returns the newly fetched block.
// - If op = write, the Access operation writes the specified data* to the block
// identified by u, and returns the old value of the block u.
//
// For readability, we rename u to address denoting the block's identifier.
OramStatus PartitionOramController::Access(Operation op_type, uint32_t address,
                                           oram_block_t* const data) {
  auto begin_access = std::chrono::high_resolution_clock::now();
  // A temporary buffer that holds the data.
  oram_block_t block;
  // Sample a new random slot id for this block.
  uint32_t new_slot_id;
  OramStatus status = oram_crypto::Cryptor::UniformRandom(
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
    status = controller->InternalAccess(op_type, address, &block, false);
  } else {
    // Invoke a dummy read.
    OramStatus status =
        controller->InternalAccess(Operation::kRead, address, nullptr, true);
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

  return OramStatus::kOK;
}

OramStatus PartitionOramController::Evict(uint32_t id) {
  logger->debug("Evicting slot {}", id);
  PathOramController* const controller = path_oram_controllers_[id].get();
  if (slots_[id].empty()) {
    // Perform a fake write.
    return controller->InternalAccess(Operation::kWrite, 0, nullptr, true);
  } else {
    logger->debug("---------------EVICT------------------");
    oram_utils::PrintStash(slots_[id]);
    logger->debug("---------------EVICT------------------");
    oram_block_t block = slots_[id].back();
    slots_[id].pop_back();
    return controller->InternalAccess(Operation::kWrite, block.header.block_id,
                                      &block, false);
  }
}

// RandomEvict samples \nu \in \mathbb{N} random slots (with replacement) to
// evict from.
OramStatus PartitionOramController::RandomEvict(void) {
  // For simplicity, we use uniform random sampling.
  for (size_t i = 0; i < nu_; i++) {
    uint32_t id;
    oram_utils::CheckStatus(oram_crypto::Cryptor::UniformRandom(
                                0, path_oram_controllers_.size() - 1, &id),
                            "Failed to sample a new slot id.");
    if (Evict(id) != OramStatus::kOK) {
      return OramStatus::kInvalidOperation;
    }
  }

  return OramStatus::kOK;
}

// SequentialEvict determines the number of blocks to evict num based on a
// prescribed dis- tribution D(ν) and sequentially scans num slots to evict
// from. RandomEvict samples ν ∈ N random slots (with replacement) to evict
// from.
OramStatus PartitionOramController::SequentialEvict(void) {
  size_t evict_num;
  oram_utils::CheckStatus(
      oram_crypto::Cryptor::UniformRandom(0, path_oram_controllers_.size() - 1,
                                          (uint32_t*)&evict_num),
      "Failed to sample eviction number.");
  for (size_t i = 0; i < evict_num; i++) {
    // cnt is a global counter for the sequential scan.
    counter_ = (counter_ + 1) % partition_size_;
    if (Evict(counter_) != OramStatus::kOK) {
      return OramStatus::kInvalidOperation;
    }
  }

  return OramStatus::kOK;
}

OramStatus PartitionOramController::Run(uint32_t block_num,
                                        uint32_t bucket_size) {
  logger->info("[+] The Partition Oram Controller is running...");
  // Determine the size of each sub-ORAM and the number of slot number.
  const size_t squared = std::ceil(std::sqrt(block_num));
  partition_size_ = std::ceil(squared * (1));
  block_num_ = block_num;
  bucket_size_ = bucket_size;

  logger->debug("The Partition ORAM's config: partition_size = {} ",
                partition_size_);
  // Initialize all the slots.
  slots_.resize(squared);

  return InitOram();
}

OramStatus PartitionOramController::InitOram(void) {
  for (size_t i = 0; i < slots_.size(); i++) {
    // We create the PathORAM controller for each slot.
    path_oram_controllers_.emplace_back(std::make_unique<PathOramController>(
        i, partition_size_, bucket_size_, false));
    path_oram_controllers_.back()->SetStub(stub_);

    // Then invoke the intialization procedure.
    OramStatus status = path_oram_controllers_.back()->InitOram();
    if (status != OramStatus::kOK) {
      return status;
    }
  }

  return OramStatus::kOK;
}

OramStatus PartitionOramController::ProcessSlot(
    const std::vector<oram_block_t>& data, uint32_t slot_id) {
  // Initialize the position map.
  std::for_each(data.begin(), data.end(), [&](const oram_block_t& block) {
    if (block.header.type == BlockType::kNormal) {
      position_map_[block.header.block_id] = slot_id;
    }
  });

  return OramStatus::kOK;
}

OramStatus PartitionOramController::FillWithData(
    const std::vector<oram_block_t>& data) {
  const size_t level = path_oram_controllers_.front()->GetTreeLevel();
  const size_t tree_size = (POW2(level + 1) - 1) * bucket_size_;
  // Check if the data size is consistent with the block number (note that this
  // includes dummy blocks).
  if (data.size() != tree_size * path_oram_controllers_.size()) {
    return OramStatus::kInvalidArgument;
  }

  // Send the data vector to each PathORAM controller.
  auto begin = std::chrono::high_resolution_clock::now();
  for (size_t i = 0; i < path_oram_controllers_.size(); i++) {
    // Slice the data vector.
    const std::vector<oram_block_t> cur_data(
        data.begin() + i * tree_size, data.begin() + (i + 1) * tree_size);
    OramStatus status = ProcessSlot(cur_data, i);
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

  // Set initialized.
  is_initialized_ = true;

  return OramStatus::kOK;
}

OramStatus PartitionOramController::TestPathOram(uint32_t controller_id) {
  if (controller_id >= path_oram_controllers_.size()) {
    logger->error("The controller id is out of range.");
    return OramStatus::kOutOfRange;
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
    OramStatus status =
        controller->InternalAccess(Operation::kRead, i, &block, false);
    oram_utils::CheckStatus(status, "Failed to read block.");

    logger->debug("[+] Read block {}: {}", block.header.block_id,
                  block.data[0]);
  }

  for (size_t i = 0; i < partition_size_; i++) {
    oram_block_t block;
    block.header.block_id = i;
    block.header.type = BlockType::kNormal;
    block.data[0] = partition_size_ - i;
    OramStatus status =
        controller->InternalAccess(Operation::kWrite, i, &block, false);
    oram_utils::CheckStatus(status, "Failed to write block.");
  }

  for (size_t i = 0; i < partition_size_; i++) {
    oram_block_t block;
    OramStatus status =
        controller->InternalAccess(Operation::kRead, i, &block, false);
    oram_utils::CheckStatus(status, "Failed to read block.");

    logger->debug("[+] Read block {}: {}", block.header.block_id,
                  block.data[0]);
  }

  auto end = std::chrono::high_resolution_clock::now();
  logger->info(
      "[-] End Testing Path ORAM. Time elapsed: {} ms.",
      std::chrono::duration_cast<std::chrono::milliseconds>(end - begin)
          .count());

  return OramStatus::kOK;
}

OramStatus PartitionOramController::TestPartitionOram(void) {
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
  OramStatus status = FillWithData(blocks);
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
    OramStatus status = Access(Operation::kRead, i, &block);
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

  return OramStatus::kOK;
}

size_t PartitionOramController::ReportClientStorage(void) const {
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

std::chrono::microseconds PartitionOramController::ReportNetworkingTime(
    void) const {
  std::chrono::microseconds ans = 0us;

  for (const auto& controller : path_oram_controllers_) {
    ans += controller->ReportNetworkingTime();
  }

  return ans;
}

size_t PartitionOramController::ReportNetworkCommunication(void) const {
  size_t ans = 0;

  for (const auto& controller : path_oram_controllers_) {
    ans += controller->ReportNetworkCommunication();
  }

  return ans;
}

}  // namespace oram_impl