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
#ifndef PARTITION_ORAM_SERVER_ORAM_STORAGE_H_
#define PARTITION_ORAM_SERVER_ORAM_STORAGE_H_

#include <cstdint>
#include <cstddef>
#include <cmath>
#include <unordered_map>

#include "base/oram_defs.h"
#include "protos/messages.pb.h"

namespace partition_oram {
class OramServerStorage {
  server_storage_t storage_;
  // The id corresponding to each slot.
  uint32_t id_;
  // How many buckets are in the storage.
  size_t capacity_;
  // The level of the oram tree.
  uint32_t level_;
  // The size of each bucket.
  size_t bucket_size_;

 public:
  OramServerStorage(uint32_t id, size_t capacity, size_t bucket_size);

  Status ReadPath(uint32_t level, uint32_t path,
                  p_oram_bucket_t* const out_bucket);
  Status WritePath(uint32_t level, uint32_t path,
                   const p_oram_bucket_t& in_bucket);
  Status AccurateWritePath(uint32_t level, uint32_t offset,
                           const p_oram_bucket_t& in_bucket,
                           partition_oram::Type type);

  server_storage_t get_storage(void) const { return storage_; }

  float ReportStorage(void) const;
};
}  // namespace partition_oram

#endif  // PARTITION_ORAM_SERVER_ORAM_STORAGE_H_