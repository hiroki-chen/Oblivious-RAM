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
#ifndef ORAM_IMPL_SERVER_TREE_ORAM_STORAGE_H_
#define ORAM_IMPL_SERVER_TREE_ORAM_STORAGE_H_

#include "base_oram_storage.h"

#include "base/oram_status.h"
#include "protos/messages.pb.h"

namespace oram_impl {
class TreeOramServerStorage : public BaseOramServerStorage {
  server_tree_storage_t storage_;
  // The level of the oram tree.
  uint32_t level_;
  // The size of each bucket.
  size_t bucket_size_;

 public:
  TreeOramServerStorage(uint32_t id, size_t capacity, size_t block_size,
                        size_t bucket_size, const std::string& instance_hash);

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

#endif  // ORAM_IMPL_SERVER_TREE_ORAM_STORAGE_H_