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
#ifndef ORAM_IMPL_CORE_ORAM_CONTROLLER_H_
#define ORAM_IMPL_CORE_ORAM_CONTROLLER_H_

#include <grpc++/grpc++.h>

#include <chrono>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "base/oram_crypto.h"
#include "base/oram_utils.h"
#include "base/oram_defs.h"
#include "protos/messages.grpc.pb.h"

namespace oram_impl {
class OramController {
 protected:
  const uint32_t id_;
  // Whether this pathoram conroller is a standalone controller
  // or embedded with other controllers, say PartitionORAM controller.
  const bool standalone_;
  // The number can vary.
  size_t block_num_;
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
  OramController(uint32_t id, bool standalone, size_t block_num,
                 OramType oram_type);

  virtual OramStatus InitOram(void) = 0;
  virtual OramStatus FillWithData(const std::vector<oram_block_t>& data) = 0;
  virtual OramStatus Access(Operation op_type, uint32_t address,
                            oram_block_t* const data) {
    return InternalAccess(op_type, address, data, false);
  }
  virtual OramStatus FromFile(const std::string& file_path);
  virtual uint32_t RandomPosition(void) { return 0ul; }

  virtual uint32_t GetId(void) const { return id_; }
  virtual OramType GetOramType(void) const { return oram_type_; }
  virtual size_t GetBlockNum(void) const { return block_num_; }
  virtual bool IsStandAlone(void) const { return standalone_; }

  virtual void SetStub(std::shared_ptr<oram_server::Stub> stub) {
    stub_ = stub;
  }

  virtual std::string GetName(void) const {
    return oram_utils::TypeToName(oram_type_);
  }

  virtual ~OramController() {
    // Because they are shared pointers, we cannot directly drop them.
    stub_.reset();
    cryptor_.reset();
  }
};

}  // namespace oram_impl

#endif  // ORAM_IMPL_CORE_ORAM_CONTROLLER_H_