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
#ifndef ORAM_IMPL_BASE_ODS_DEFS_H_
#define ORAM_IMPL_BASE_ODS_DEFS_H_

#include <cstdint>
#include <string>

#ifndef MAX
#define MAX(lhs, rhs) (lhs >= rhs ? lhs : rhs)
#endif

#ifndef HEIGHT
#define HEIGHT(x) \
  (x == nullptr ? 0 : MAX(x->left_height_, x->right_height_) + 1)
#endif

namespace oram_impl::ods {
static const uint32_t deserialized_str_size = 13ul;
static const uint32_t invalid_mask = 0ul;

static const std::string pad_val_err =
    "Pad value is smaller than the read count!";

enum class OdsOperation {
  kRead = 1,
  kWrite = 2,
  kRemove = 3,
  kInsert = 4,
  kInvalid = 5,
};

}  // namespace oram_impl::ods

#endif