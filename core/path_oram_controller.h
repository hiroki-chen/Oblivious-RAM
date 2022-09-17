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
#ifndef ORAM_IMPL_CORE_PATH_ORAM_CONTROLLER_H_
#define ORAM_IMPL_CORE_PATH_ORAM_CONTROLLER_H_

#include "oram_controller.h"

namespace oram_impl {

// This class is the implementation of the ORAM controller for Path ORAM.
class PathOramController : public OramController {
  friend class PartitionOramController;

  // ORAM parameters.
  uint32_t tree_level_;
  uint8_t bucket_size_;
  uint32_t number_of_leafs_;
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
  // The separate interface for reading is reserved for direct designation of
  // position, which is useful to, say, ODS.
  virtual OramStatus InternalAccessDirect(Operation op_type, uint32_t address,
                                          uint32_t position,
                                          oram_block_t* const data,
                                          bool dummy = false);

 public:
  PathOramController(uint32_t id, uint32_t block_num, uint32_t bucket_size,
                     bool standalone = true);

  virtual OramStatus InitOram(void) override;
  virtual OramStatus FillWithData(
      const std::vector<oram_block_t>& data) override;
  virtual uint32_t RandomPosition(void) override;

  virtual OramStatus AccessDirect(Operation op_type, uint32_t address,
                                  uint32_t position, oram_block_t* const data) {
    return !is_initialized_
               ? OramStatus(StatusCode::kInvalidOperation,
                            "Cannot access ORAM before it is initialized")
               : InternalAccessDirect(op_type, address, position, data, false);
  }

  p_oram_position_t GetPositionMap(void) const { return position_map_; }
  uint32_t GetTreeLevel(void) const { return tree_level_; }
  size_t ReportClientStorage(void) const;
  size_t ReportStashSize(void) const { return stash_size_; }
  size_t ReportNetworkCommunication(void) const;
  std::chrono::microseconds ReportNetworkingTime(void) const {
    return network_time_;
  }
};
}  // namespace oram_impl

#endif  // ORAM_IMPL_CORE_PATH_ORAM_CONTROLLER_H_