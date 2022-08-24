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

 public:
  BaseOramServerStorage(uint32_t id, size_t capacity,
                        OramStorageType oram_storage_type)
      : id_(id), capacity_(capacity), oram_storage_type_(oram_storage_type) {}

  virtual uint32_t GetId(void) const { return id_; }
  virtual size_t GetCapacity(void) const { return capacity_; }
  virtual OramStorageType GetOramStorageType(void) const {
    return oram_storage_type_;
  }
};

class FlatOramServerStorage : public BaseOramServerStorage {
  server_flat_storage_t storage_;

 public:
  FlatOramServerStorage(uint32_t id, size_t capacity)
      : BaseOramServerStorage(id, capacity, OramStorageType::kFlatStorage) {}

  server_flat_storage_t GetStorage(void) { return storage_; }
};

class TreeOramServerStorage : public BaseOramServerStorage {
  server_tree_storage_t storage_;
  // The level of the oram tree.
  uint32_t level_;
  // The size of each bucket.
  size_t bucket_size_;

 public:
  TreeOramServerStorage(uint32_t id, size_t capacity, size_t bucket_size);

  OramStatus ReadPath(uint32_t level, uint32_t path,
                      p_oram_bucket_t* const out_bucket);
  OramStatus WritePath(uint32_t level, uint32_t path,
                       const p_oram_bucket_t& in_bucket);
  OramStatus AccurateWritePath(uint32_t level, uint32_t offset,
                               const p_oram_bucket_t& in_bucket,
                               oram_impl::Type type);

  server_tree_storage_t GetStorage(void) const { return storage_; }

  float ReportStorage(void) const;
};
}  // namespace oram_impl

#endif  // ORAM_IMPL_SERVER_ORAM_STORAGE_H_