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
#ifndef ORAM_IMPL_SERVER_BASE_ORAM_STORAGE_H_
#define ORAM_IMPL_SERVER_BASE_ORAM_STORAGE_H_

#include <cstdint>

#include "base/oram_defs.h"

namespace oram_impl {
class BaseOramServerStorage {
 protected:
  // The id.
  const uint32_t id_;
  // How many buckets / blocks it can hold.
  const size_t capacity_;
  // The type.
  OramStorageType oram_storage_type_;
  // The size of the block.
  const size_t block_size_;
  // The hash of the instance.
  const std::string instance_hash_;

 public:
  BaseOramServerStorage(uint32_t id, size_t capacity, size_t block_size,
                        const std::string& instance_hash,
                        OramStorageType oram_storage_type)
      : id_(id),
        capacity_(capacity),
        oram_storage_type_(oram_storage_type),
        block_size_(block_size),
        instance_hash_(instance_hash) {}

  virtual uint32_t GetId(void) const { return id_; }
  virtual size_t GetCapacity(void) const { return capacity_; }
  virtual OramStorageType GetOramStorageType(void) const {
    return oram_storage_type_;
  }
  virtual std::string GetInstanceHash(void) const { return instance_hash_; }
  virtual size_t GetBlockSize(void) const { return block_size_; }
  virtual float ReportStorage(void) const { return 0.0; }

  virtual ~BaseOramServerStorage() = 0;
};
}  // namespace oram_impl

#endif