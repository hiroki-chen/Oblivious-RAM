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
#ifndef ORAM_IMPL_CLIENT_ORAM_CLIENT_H_
#define ORAM_IMPL_CLIENT_ORAM_CLIENT_H_

#include "core/oram.h"

namespace oram_impl {
class OramClient {
  std::unique_ptr<OramController> oram_controller_;

 public:
  // The only way to construct an ORAM client.
  OramClient(const OramConfig& config);

  // READ / WRITE Interfaces.
  OramStatus Read(uint32_t address, oram_block_t* const block) {
    return oram_controller_->Access(Operation::kRead, address, block);
  }
  OramStatus Write(uint32_t address, oram_block_t* const block) {
    return oram_controller_->Access(Operation::kWrite, address, block);
  }
};
}  // namespace oram_impl

#endif  // ORAM_IMPL_CLIENT_ORAM_CLIENT_H_
