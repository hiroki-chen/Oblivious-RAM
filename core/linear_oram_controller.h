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
#ifndef ORAM_IMPL_CORE_LINEAR_ORAM_CONTROLLER_H_
#define ORAM_IMPL_CORE_LINEAR_ORAM_CONTROLLER_H_

#include "oram_controller.h"

#include "base/oram_status.h"

namespace oram_impl {
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
  LinearOramController(uint32_t id, bool standalone, size_t block_num)
      : OramController(id, standalone, block_num, OramType::kLinearOram) {}
  virtual OramStatus InitOram(void) override;
  virtual OramStatus FillWithData(
      const std::vector<oram_block_t>& data) override;
};
} // namespace oram_impl

#endif // ORAM_IMPL_CORE_LINEAR_ORAM_CONTROLLER_H_