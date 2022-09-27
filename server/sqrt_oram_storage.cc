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
      return pos < shelter_.size();
    case 1:
      return pos < main_memory_.size();
    case 2:
      return pos < dummy_.size();
    default:
      return false;
  }
}
}  // namespace oram_impl