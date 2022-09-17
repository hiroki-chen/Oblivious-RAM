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
#ifndef ORAM_IMPL_CORE_SQUARE_ROOT_ORAM_CONTROLLER_H_
#define ORAM_IMPL_CORE_SQUARE_ROOT_ORAM_CONTROLLER_H_

#include "oram_controller.h"

namespace oram_impl {
class SquareRootOramController : public OramController {
  // The layout of the square root ORAM is:
  // ------------------ | ------- | -------|
  //    m main sotrage    sqrt(m)   sqrt(m)
  //                      dummy     shelter
  // |       permuted memory      |
  //
  // Select a permutation \pi over the integers 1, ..., m + \sqrt{m} and
  // obliviously relocate the words according to the permutation.
  //
  // In our implementation, we make two modifications:
  // 1. We will use an alternative algorithm for achieving pseudorandom
  // permutation: the format-preserving encryption that maps each value within
  // the plaintext domain.
  // 2. We will locally maintain the position map with O(sizeof(uint32_t) * N)
  // storage.
  
  size_t sqrt_m_;

  p_oram_position_t position_map_;

  // After m operations, we will permute the whole ORAM storage.
  uint32_t counter_;
 protected:
  virtual OramStatus InternalAccess(Operation op_type, uint32_t address,
                                    oram_block_t* const data,
                                    bool dummy = false) override;

  virtual OramStatus ReadBlock(uint32_t position, oram_block_t* const data);
  virtual OramStatus WriteBlock(uint32_t position, oram_block_t* const data);
  virtual OramStatus PermuteOnFull(void);

 public:
  // For the convenience of format-preserving encryption, we will round up the
  // block_num to a value such that m + \sqrt{m} =  2^{n}, for some integer n.
  SquareRootOramController(uint32_t id, bool standalone, size_t block_num);
};
}  // namespace oram_impl

#endif  // ORAM_IMPL_CORE_SQUARE_ROOT_ORAM_CONTROLLER_H_
