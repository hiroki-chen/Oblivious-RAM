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
#ifndef ORAM_IMPL_CLIENT_ORAM_CONTROLLER_H_
#define ORAM_IMPL_CLIENT_ORAM_CONTROLLER_H_

#include <grpc++/grpc++.h>

#include <chrono>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "base/oram_crypto.h"
#include "base/oram_defs.h"
#include "protos/messages.grpc.pb.h"

namespace oram_impl {
class OramController {
 protected:
  const uint32_t id_;
  // Whether this pathoram conroller is a standalone controller
  // or embedded with other controllers, say PartitionORAM controller.
  const bool standalone_;
  // Oram type.
  const OramType oram_type_;

  // An object used to call some methods of ORAM storage on the cloud.
  std::shared_ptr<oram_server::Stub> stub_;
  // Cryptography manager.
  std::shared_ptr<oram_crypto::Cryptor> cryptor_;

  // This interface is reserved for other ORAMs that use this ORAM controller as
  // its backbone; sometimes the high-level ORAM will need to perform some fake
  // operations (e.g., the Partition ORAM).
  virtual OramStatus InternalAccess(Operation op_type, uint32_t address,
                                    oram_block_t* const data,
                                    bool dummy = false) = 0;

 public:
  OramController(uint32_t id, bool standalone, OramType oram_type);

  virtual OramStatus InitOram(void) = 0;
  virtual OramStatus FillWithData(const std::vector<oram_block_t>& data) = 0;
  virtual OramStatus Access(Operation op_type, uint32_t address,
                            oram_block_t* const data) {
    return InternalAccess(op_type, address, data, false);
  }
  virtual OramStatus FromFile(const std::string& file_path) {
    UNIMPLEMENTED_FUNC;
  }

  virtual uint32_t GetId(void) const { return id_; }
  virtual OramType GetOramType(void) const { return oram_type_; }
  virtual bool IsStandAlone(void) const { return standalone_; }
  virtual void SetStub(std::shared_ptr<oram_server::Stub> stub) {
    stub_ = stub;
  }

  virtual ~OramController() {
    // Because they are shared pointers, we cannot directly drop them.
    stub_.reset();
    cryptor_.reset();
  }
};

// A trivial solution for ORAM. It only has educational meaning. Do not use it
// in any productive environment.
class LinearOramController : public OramController {
 private:
  grpc::Status ReadFromServer(std::string* const out);
  grpc::Status WriteToServer(const std::string& input);

 protected:
  virtual OramStatus InternalAccess(Operation op_type, uint32_t address,
                                    oram_block_t* const data,
                                    bool dummy = false) override;

 public:
  LinearOramController(uint32_t id, bool standalone)
      : OramController(id, standalone, OramType::kLinearOram) {}
};

class SquareRootOramController : public OramController {
 protected:
  virtual OramStatus InternalAccess(Operation op_type, uint32_t address,
                                    oram_block_t* const data,
                                    bool dummy = false) override;

 public:
  SquareRootOramController(uint32_t id, bool standalone)
      : OramController(id, standalone, OramType::kSquareOram) {}
};

// This class is the implementation of the ORAM controller for Path ORAM.
class PathOramController : public OramController {
  friend class PartitionOramController;

  uint32_t id_;
  // ORAM parameters.
  uint32_t number_of_leafs_;
  uint32_t tree_level_;
  uint8_t bucket_size_;

  // stash size.
  size_t stash_size_;

  p_oram_position_t position_map_;
  // The stash should be tied to the slots of Partition ORAM, so we use
  // pointers to manipulate the stash.
  p_oram_stash_t stash_;
  // Networking time.
  std::chrono::microseconds network_time_;
  // Networking communication.
  size_t network_communication_;

  // ==================== Begin private methods ==================== //
  OramStatus ReadBucket(uint32_t path, uint32_t level,
                        p_oram_bucket_t* const bucket);
  OramStatus WriteBucket(uint32_t path, uint32_t level,
                         const p_oram_bucket_t& bucket);
  OramStatus AccurateWriteBucket(uint32_t level, uint32_t offset,
                                 const p_oram_bucket_t& bucket);
  OramStatus PrintOramTree(void);

  p_oram_stash_t FindSubsetOf(uint32_t current_path);
  // ==================== End private methods ==================== //
 protected:
  virtual OramStatus InternalAccess(Operation op_type, uint32_t address,
                                    oram_block_t* const data,
                                    bool dummy = false);

 public:
  PathOramController(uint32_t id, uint32_t block_num, uint32_t bucket_size,
                     bool standalone = true);

  virtual OramStatus InitOram(void) override;
  virtual OramStatus FillWithData(
      const std::vector<oram_block_t>& data) override;
  virtual OramStatus FromFile(const std::string& file_path) override;

  uint32_t GetTreeLevel(void) const { return tree_level_; }
  size_t ReportClientStorage(void) const;
  size_t ReportStashSize(void) const { return stash_size_; }
  size_t ReportNetworkCommunication(void) const;
  std::chrono::microseconds ReportNetworkingTime(void) const {
    return network_time_;
  }
};

// This class is the implementation of the ORAM controller for Partition ORAM.
class PartitionOramController final : public OramController {
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
  // TODO: Currently this is pathoram controller.
  std::vector<std::unique_ptr<PathOramController>> path_oram_controllers_;

  PartitionOramController(uint32_t id = 0ul)
      : OramController(id, true, OramType::kPartitionOram) {}

  // ==================== Begin private methods ==================== //
  OramStatus Evict(uint32_t id);
  OramStatus SequentialEvict(void);
  OramStatus RandomEvict(void);

  OramStatus ProcessSlot(const std::vector<oram_block_t>& data,
                         uint32_t slot_id);
  // ==================== End private methods ==================== //
 protected:
  virtual OramStatus InternalAccess(Operation op_type, uint32_t address,
                                    oram_block_t* const data,
                                    bool dummy = false) {
    UNIMPLEMENTED_FUNC;
  }

 public:
  static std::unique_ptr<PartitionOramController> GetInstance();

  void SetBucketSize(size_t bucket_size) { bucket_size_ = bucket_size; }
  void SetNu(size_t nu) { nu_ = nu; }

  virtual OramStatus Access(Operation op_type, uint32_t address,
                            oram_block_t* const data) override;

  virtual OramStatus FillWithData(
      const std::vector<oram_block_t>& data) override;
  virtual OramStatus InitOram(void) override;

  OramStatus Run(uint32_t block_num, uint32_t bucket_size);
  // A reserved interface for testing one of the PathORAM controllers.
  OramStatus TestPathOram(uint32_t controller_id);
  OramStatus TestPartitionOram(void);

  size_t ReportClientStorage(void) const;
  size_t ReportNetworkCommunication(void) const;
  std::chrono::microseconds ReportNetworkingTime(void) const;

  void Reset(uint32_t block_num) {
    block_num_ = block_num;
    position_map_.clear();
    slots_.clear();
    path_oram_controllers_.clear();
  }

  virtual ~PartitionOramController() {}
};
}  // namespace oram_impl

#endif  // ORAM_IMPL_CLIENT_ORAM_CONTROLLER_H_