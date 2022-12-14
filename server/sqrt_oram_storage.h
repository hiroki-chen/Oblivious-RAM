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
#ifndef ORAM_IMPL_SERVER_SQRT_ORAM_STORAGE_H_
#define ORAM_IMPL_SERVER_SQRT_ORAM_STORAGE_H_

#include "base_oram_storage.h"

namespace oram_impl {
class SqrtOramServerStorage : public BaseOramServerStorage {
  server_sqrt_storage_t main_memory_;
  // Shelter is a vector of [tag, data]. The tag field is used to track the
  // blocks in the main memory in case we need to update them.
  server_sqrt_shelter_t shelter_;
  server_sqrt_storage_t dummy_;

  size_t squared_m_;

 public:
  SqrtOramServerStorage(uint32_t id, size_t capacity, size_t block_size,
                        size_t squared_m, const std::string& instance_hash)
      : BaseOramServerStorage(id, capacity, block_size, instance_hash,
                              OramStorageType::kSqrtStorage),
        squared_m_(squared_m) {}
  bool Check(uint32_t pos, uint32_t type);
  // Position may need to "shrink to fit".
  std::string ReadBlockFromShelter(uint32_t pos);
  std::string ReadBlockFromMain(uint32_t pos);
  std::string ReadBlockFromDummy(uint32_t pos);
  void WriteBlockToShelter(uint32_t tag, const std::string& data);
  void Fill(const std::vector<std::string>& data);
  void DoPermute(const std::vector<uint32_t>& perm);
};
}  // namespace oram_impl

#endif  // ORAM_IMPL_SERVER_SQRT_ORAM_STORAGE_