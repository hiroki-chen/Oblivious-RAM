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
SquareRootOramController::SquareRootOramController(uint32_t id, bool standalone,
                                                   size_t block_num)
    : OramController(id, standalone, block_num, OramType::kSquareOram) {
  const size_t m_squared = (size_t)(std::ceil(std::sqrt(block_num)));
  shelter_.reserve(m_squared);
}

OramStatus SquareRootOramController::InternalAccess(Operation op_type,
                                                    uint32_t address,
                                                    oram_block_t* const data,
                                                    bool dummy) {
  return OramStatus::kOK;
}
}  // namespace oram_impl