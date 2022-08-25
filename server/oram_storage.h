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
#ifndef ORAM_IMPL_SERVER_ORAM_STORAGE_H_
#define ORAM_IMPL_SERVER_ORAM_STORAGE_H_

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <unordered_map>

#include "base/oram_defs.h"
#include "protos/messages.pb.h"

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

 public:
  BaseOramServerStorage(uint32_t id, size_t capacity, size_t block_size,
                        OramStorageType oram_storage_type)
      : id_(id),
        capacity_(capacity),
        block_size_(block_size),
        oram_storage_type_(oram_storage_type) {}

  virtual uint32_t GetId(void) const { return id_; }
  virtual size_t GetCapacity(void) const { return capacity_; }
  virtual OramStorageType GetOramStorageType(void) const {
    return oram_storage_type_;
  }
  virtual size_t GetBlockSize(void) const { return block_size_; }
  virtual float ReportStorage(void) const { UNIMPLEMENTED_FUNC; }

  virtual ~BaseOramServerStorage() = 0;
};

class FlatOramServerStorage : public BaseOramServerStorage {
  server_flat_storage_t storage_;

 public:
  FlatOramServerStorage(uint32_t id, size_t capacity, size_t block_size)
      : BaseOramServerStorage(id, capacity, block_size,
                              OramStorageType::kFlatStorage) {}

  virtual server_flat_storage_t GetStorage(void) { return storage_; }
  virtual void ResetStorage(void) { storage_.clear(); }
  virtual void From(const server_flat_storage_t& storage) {
    storage_ = storage;
  }
};

class TreeOramServerStorage : public BaseOramServerStorage {
  server_tree_storage_t storage_;
  // The level of the oram tree.
  uint32_t level_;
  // The size of each bucket.
  size_t bucket_size_;

 public:
  TreeOramServerStorage(uint32_t id, size_t capacity, size_t block_size,
                        size_t bucket_size);

  OramStatus ReadPath(uint32_t level, uint32_t path,
                      p_oram_bucket_t* const out_bucket);
  OramStatus WritePath(uint32_t level, uint32_t path,
                       const p_oram_bucket_t& in_bucket);
  OramStatus AccurateWritePath(uint32_t level, uint32_t offset,
                               const p_oram_bucket_t& in_bucket,
                               oram_impl::Type type);

  server_tree_storage_t GetStorage(void) const { return storage_; }

  virtual float ReportStorage(void) const;
};
}  // namespace oram_impl

#endif  // ORAM_IMPL_SERVER_ORAM_STORAGE_H_