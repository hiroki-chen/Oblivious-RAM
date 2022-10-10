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
#include "sqrt_oram_storage.h"

#include <spdlog/spdlog.h>

#include "base/oram_utils.h"

extern std::shared_ptr<spdlog::logger> logger;

namespace oram_impl {
bool SqrtOramServerStorage::Check(uint32_t pos, uint32_t type) {
  switch (type) {
    case 0:
      return true;

    case 1:
      return pos <= main_memory_.size() + dummy_.size();

    case 2:
      return true;

    default:
      return false;
  }
}

void SqrtOramServerStorage::DoPermute(const std::vector<uint32_t>& perm) {
  PANIC_IF(perm.size() != main_memory_.size() + dummy_.size(),
           "The permutation size is wrong!");

  // Server needs to reconstruct the memory layout. In short, it has two tasks:
  //    1. Update the memory using shelter's information. Since we have stored
  //       tag information in the shelter, we can easily update the main memory.
  //    2. Permute the main memory according to the uploaded data provided by
  //       the client.

  // Step 1: Update the memory.
  for (size_t i = 0; i < shelter_.size(); i++) {
    const uint32_t tag = shelter_[i].first;
    const std::string data = shelter_[i].second;

    if (tag != kInvalidMask && !data.empty()) {
      // Check which one should we update.
      if (tag < main_memory_.size()) {
        main_memory_[tag] = data;
      } else {
        dummy_[tag - main_memory_.size()] = data;
      }
    }

    // Clear the shelter.
    shelter_[i] = std::make_pair(kInvalidMask, "");
  }

  // Step 2: Permute the memory.
  // This permutation is done at the tag level.
  std::vector<std::string> assembled_arr = main_memory_;
  assembled_arr.insert(assembled_arr.end(), dummy_.begin(), dummy_.end());
  oram_utils::PermuteBy(perm, assembled_arr);

  // Copy it back.
  for (size_t i = 0; i < assembled_arr.size(); i++) {
    if (i < main_memory_.size()) {
      main_memory_[i] = assembled_arr[i];
    } else {
      dummy_[i - main_memory_.size()] = assembled_arr[i];
    }
  }
}

void SqrtOramServerStorage::WriteBlockToShelter(uint32_t tag,
                                                const std::string& data) {
  // Scan and find an available space.
  for (size_t i = 0; i < shelter_.size(); i++) {
    if (shelter_[i].second.empty() && shelter_[i].first == kInvalidMask) {
      shelter_[i].first = tag;
      shelter_[i].second = data;

      return;
    }
  }

  PANIC("No available space in the shelter. This is an internal logic error.");
}

std::string SqrtOramServerStorage::ReadBlockFromShelter(uint32_t pos) {
  const std::string ans = shelter_[pos].second;
  shelter_[pos].first = kInvalidMask;
  shelter_[pos].second.clear();
  return ans;
}

std::string SqrtOramServerStorage::ReadBlockFromMain(uint32_t pos) {
  if (pos < main_memory_.size()) {
    return main_memory_[pos];
  } else {
    return ReadBlockFromDummy(pos - main_memory_.size());
  }
}

std::string SqrtOramServerStorage::ReadBlockFromDummy(uint32_t pos) {
  // It is meaningless to remove a block from dummy. So we keep it.
  // There is no offset anymore.
  return dummy_[pos];
}

void SqrtOramServerStorage::Fill(const std::vector<std::string>& data) {
  for (size_t i = 0; i < data.size(); i++) {
    if (i < capacity_) {
      main_memory_.emplace_back(data[i]);
    } else {
      dummy_.emplace_back(data[i]);
      // Initialize the shelter.
      shelter_.emplace_back(std::make_pair(kInvalidMask, ""));
    }
  }
}

}  // namespace oram_impl