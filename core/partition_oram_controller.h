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
#ifndef ORAM_IMPL_CORE_PARTITION_ORAM_CONTROLLER_H_
#define ORAM_IMPL_CORE_PARTITION_ORAM_CONTROLLER_H_

#include "oram_controller.h"
#include "path_oram_controller.h"

namespace oram_impl {
// This class is the implementation of the ORAM controller for Partition ORAM.
class PartitionOramController final : public OramController {
  size_t partition_size_;
  size_t bucket_size_;
  size_t nu_;
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
      : OramController(id, true, 0ul, OramType::kPartitionOram) {}

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

#endif // ORAM_IMPL_CORE_PARTITION_ORAM_CONTROLLER_H_