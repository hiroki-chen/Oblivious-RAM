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
#include "square_root_oram_controller.h"

namespace oram_impl {

static std::vector<uint32_t> CreateVec(size_t size) {
  std::vector<uint32_t> ans(size, 0);

  for (size_t i = 0; i < size; i++) {
    ans[i] = i;
  }

  return ans;
}

OramStatus SquareRootOramController::ReadBlock(uint32_t position,
                                               oram_block_t* const data) {
  // TODO.
  return OramStatus::OK;
}

OramStatus SquareRootOramController::WriteBlock(uint32_t position,
                                                oram_block_t* const data) {
  return OramStatus::OK;
}

SquareRootOramController::SquareRootOramController(uint32_t id, bool standalone,
                                                   size_t block_num)
    : OramController(id, standalone, block_num, OramType::kSquareOram),
      sqrt_m_((size_t)std::ceil(std::sqrt(block_num))),
      counter_(0ul) {
  // TODO: Should check m + \sqrt{m} = log_2(x), for some x.
}

OramStatus SquareRootOramController::PermuteOnFull(void) {
  OramStatus status = OramStatus::OK;
  // Increment the counter and check if we need to permute the oram again.
  // The permutation is done on the server side; we just need to update the
  // position map.
  if (counter_++ == sqrt_m_) {
    // First let us clear the counter.
    counter_ = 0;

    std::vector<uint32_t> perm = std::move(CreateVec(block_num_ + sqrt_m_));
    status = oram_crypto::RandomPermutation(perm);
    if (!status.ok()) {
      return OramStatus(status.ErrorCode(),
                        oram_utils::StrCat("Permutation failed due to ",
                                           status.ErrorMessage()));
    }

    // TODO: Send the vector to the server.
  }

  // Nothing happens or the permutation is done correctly.
  return status;
}

OramStatus SquareRootOramController::InternalAccess(Operation op_type,
                                                    uint32_t address,
                                                    oram_block_t* const data,
                                                    bool dummy) {
  // Step 1: Randomly permute the contents of locations 1 through m + \sqrt{m}.
  // That is, select a permutation π over the integers 1 through m + \sqrt{m}
  // and relocate the contents of word i into word π(i) (later, we show how to
  // do this obliviously).

  // Step 2: Simulate \sqrt{m} memory accesses of the original RAM.
  // - If i is found in the shelter, then read next dummy.
  // - If is not found in the shelter, then read main memory using its permuted
  //   tag \pi(i) which is then stored in the shelter temporarily.

  // Step 3: After \sqrt{m} operations, re-shuffle the memory content and write
  // back.
  //
  // Note that in the original Ostrovsky's algorithm, the permutation is
  // realized by AKS sorting network:
  //  1. Give each memory location in [0, m + \sqrt{m}] a tag by a random
  //     oracle.
  //  2. Use sorting network to sort all the memory blocks by their tags.
  //
  // Because sorting network behaves identically with different values, the
  // algorithm is oblivious.
  //
  // Workflow:
  //    Server --> Client (Read & Update) -->
  //               Server stores the block in the shelter --> (Permute)

  // Check the position map.
  if (position_map_.find(address) == position_map_.end()) {
    return OramStatus(StatusCode::kInvalidArgument,
                      "The requested block does not exist!");
  }

  const uint32_t position = position_map_[address];
  oram_block_t block;
  block.header.block_id = address;
  block.header.type = BlockType::kNormal;

  // Read from the server.
  OramStatus status = OramStatus::OK;
  if (!(status = ReadBlock(position, &block)).ok()) {
    return OramStatus(status.ErrorCode(),
                      oram_utils::StrCat("[InternalAccess] Server Error: ",
                                         status.ErrorMessage()));
  }

  // Check if the id matches.
  if (block.header.block_id != address) {
    return OramStatus(
        StatusCode::kUnknownError,
        oram_utils::StrCat("[InternalAccess] The requested id ", address,
                           " does not match with the fetched block ",
                           block.header.block_id, "!"));
  }

  // Check if we need to permute the oram storage.
  if (!(status = PermuteOnFull()).ok()) {
    return OramStatus(
        status.ErrorCode(),
        oram_utils::StrCat("[InternalAccess] ", status.ErrorMessage()));
  }

  return OramStatus::OK;
}
}  // namespace oram_impl