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
#ifndef PARTITION_ORAM_CLIENT_ORAM_CONTROLLER_H_
#define PARTITION_ORAM_CLIENT_ORAM_CONTROLLER_H_

#include <chrono>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

#include <grpc++/grpc++.h>

#include "base/oram_defs.h"
#include "base/oram_crypto.h"
#include "protos/messages.grpc.pb.h"

namespace partition_oram {
// This class is the implementation of the ORAM controller for Path ORAM.
class PathOramController {
  friend class OramController;

  uint32_t id_;
  // ORAM parameters.
  uint32_t number_of_leafs_;
  uint32_t tree_level_;
  uint8_t bucket_size_;

  p_oram_position_t position_map_;
  // The stash should be tied to the slots of Partition ORAM, so we use
  // pointers to manipulate the stash.
  p_oram_stash_t stash_;
  // An object used to call some methods of ORAM storage on the cloud.
  std::shared_ptr<server::Stub> stub_;
  // Cryptography manager.
  std::shared_ptr<oram_crypto::Cryptor> cryptor_;
  // Networking time.
  std::chrono::microseconds network_time_;
  // Networking communication.
  size_t network_communication_;

  // ==================== Begin private methods ==================== //
  Status ReadBucket(uint32_t path, uint32_t level,
                    p_oram_bucket_t* const bucket);
  Status WriteBucket(uint32_t path, uint32_t level,
                     const p_oram_bucket_t& bucket);
  Status AccurateWriteBucket(uint32_t level, uint32_t offset,
                             const p_oram_bucket_t& bucket);
  Status PrintOramTree(void);

  p_oram_stash_t FindSubsetOf(uint32_t current_path);
  // ==================== End private methods ==================== //

 public:
  PathOramController(uint32_t id, uint32_t block_num, uint32_t bucket_size);

  void SetStub(std::shared_ptr<server::Stub> stub) { stub_ = stub; }

  Status InitOram(void);
  Status FillWithData(const std::vector<oram_block_t>& data);

  // The meanings of parameters are explained in Stefanov et al.'s paper.
  Status Access(Operation op_type, uint32_t address, oram_block_t* const data,
                bool dummy);

  uint32_t GetTreeLevel(void) const { return tree_level_; }
  size_t ReportClientStorage(void) const;
  size_t ReportNetworkCommunication(void) const;
  std::chrono::microseconds ReportNetworkingTime(void) const { return network_time_; }

  virtual ~PathOramController() {
    stub_.reset();
    cryptor_.reset();
  }
};

// This class is the implementation of the ORAM controller for Partition ORAM.
class OramController {
  size_t partition_size_;
  size_t bucket_size_;
  size_t nu_;
  size_t block_num_;
  static size_t counter_;
  // Position map: [key] -> [slot_id].
  p_oram_position_t position_map_;
  // Slots: [slot_id] -> [block1, block2, ..., block_n].
  pp_oram_slot_t slots_;
  // Controllers for each slot: [slot_id] -> [controller_1, controller_2, ...,
  //                                          controller_n].
  std::vector<std::unique_ptr<PathOramController>> path_oram_controllers_;
  // Cryptography manager.
  std::shared_ptr<oram_crypto::Cryptor> cryptor_;
  // Stub
  std::shared_ptr<server::Stub> stub_;


  OramController() {}

  // ==================== Begin private methods ==================== //
  Status Evict(uint32_t id);
  Status SequentialEvict(void);
  Status RandomEvict(void);

  Status ProcessSlot(const std::vector<oram_block_t>& data, uint32_t slot_id);
  // ==================== End private methods ==================== //

 public:
  static std::unique_ptr<OramController> GetInstance();

  void SetStub(std::shared_ptr<server::Stub> stub) { stub_ = stub; }
  void SetBucketSize(size_t bucket_size) { bucket_size_ = bucket_size; }
  void SetNu(size_t nu) { nu_ = nu; }

  Status Access(Operation op_type, uint32_t address, oram_block_t* const data);
  Status Run(uint32_t block_num, uint32_t bucket_size);
  Status FillWithData(const std::vector<oram_block_t>& data);

  // A reserved interface for testing one of the PathORAM controllers.
  Status TestPathOram(uint32_t controller_id);
  Status TestPartitionOram(void);

  size_t ReportClientStorage(void) const;
  size_t ReportNetworkCommunication(void) const;
  std::chrono::microseconds ReportNetworkingTime(void) const;

  void Reset(uint32_t block_num) {
    block_num_ = block_num;
    position_map_.clear();
    slots_.clear();
    path_oram_controllers_.clear();
  }

  virtual ~OramController() {}
};
}  // namespace partition_oram

#endif  // PARTITION_ORAM_CLIENT_ORAM_CONTROLLER_H_