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

#include "base/oram_utils.h"

namespace oram_impl {
bool SqrtOramServerStorage::Check(uint32_t pos, uint32_t type) {
  switch (type) {
    case 0:
      return main_memory_.size() <= pos &&
             pos < shelter_.size() + main_memory_.size();

    case 1:
      return 0 < pos && pos < main_memory_.size();

    case 2:
      return shelter_.size() + main_memory_.size() <= pos &&
             pos < shelter_.size() + main_memory_.size() + dummy_.size();

    default:
      return false;
  }
}

void SqrtOramServerStorage::DoPermute(const std::vector<uint32_t>& perm) {
  for (size_t i = 0; i < perm.size(); i++) {
    // This is the index of the element that should be relocated to the current
    // index; we need to check the boundary.
    const uint32_t target_element = perm[i];

    // Permute main memory.
    if (i < main_memory_.size()) {
      if (target_element < main_memory_.size()) {
        std::swap(main_memory_[i], main_memory_[target_element]);
      } else {
        std::swap(main_memory_[i],
                  shelter_[target_element - main_memory_.size()]);
      }
    }
    // Permute the shelter.
    else {
      if (target_element < main_memory_.size()) {
        std::swap(shelter_[i - main_memory_.size()],
                  main_memory_[target_element]);
      } else {
        std::swap(shelter_[i - main_memory_.size()],
                  shelter_[target_element - main_memory_.size()]);
      }
    }
  }
}

std::string SqrtOramServerStorage::ReadBlockFromShelter(uint32_t pos) {
  const std::string ans = shelter_[pos - main_memory_.size()];
  // Clear this position.
  shelter_[pos - main_memory_.size()].clear();
  return ans;
}

std::string SqrtOramServerStorage::ReadBlockFromMain(uint32_t pos) {
  const std::string ans = main_memory_[pos];
  main_memory_[pos].clear();
  return ans;
}

std::string SqrtOramServerStorage::ReadBlockFromDummy(uint32_t pos) {
  // It is meaningless to remove a block from dummy. So we keep it.
  return dummy_[pos - main_memory_.size() - shelter_.size()];
}

void SqrtOramServerStorage::WriteBlockToShelter(const std::string& data) {
  for (size_t i = 0; i < shelter_.size(); i++) {
    // Find an empty space and write to it.
    if (shelter_[i].empty()) {
      shelter_[i] = data;

      return;
    }
  }

  // Should always find an available space for this block.
}

void SqrtOramServerStorage::Fill(const std::vector<std::string>& data) {
  for (size_t i = 0; i < data.size(); i++) {
    if (i < capacity_) {
      main_memory_.emplace_back(data[i]);
    } else {
      shelter_.emplace_back(data[i]);
    }
  }
}

}  // namespace oram_impl