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
#ifndef ORAM_IMPL_SERVER_FLAT_ORAM_STORAGE_H_
#define ORAM_IMPL_SERVER_FLAT_ORAM_STORAGE_H_

#include "base_oram_storage.h"

namespace oram_impl {
class FlatOramServerStorage : public BaseOramServerStorage {
  server_flat_storage_t storage_;

 public:
  FlatOramServerStorage(uint32_t id, size_t capacity, size_t block_size,
                        const std::string& instance_hash)
      : BaseOramServerStorage(id, capacity, block_size, instance_hash,
                              OramStorageType::kFlatStorage) {}

  virtual server_flat_storage_t GetStorage(void) { return storage_; }
  virtual void ResetStorage(void) { storage_.clear(); }
  virtual void From(const server_flat_storage_t& storage) {
    storage_ = storage;
  }
};
}  // namespace oram_impl

#endif  // ORAM_IMPL_SERVER_FLAT_ORAM_STORAGE_H_