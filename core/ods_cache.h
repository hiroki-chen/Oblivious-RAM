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
#ifndef ORAM_IMPL_CORE_ODS_CACHE_H_
#define ORAM_IMPL_CORE_ODS_CACHE_H_

#include <cstddef>
#include <deque>
#include <iostream>
#include <map>
#include <unordered_map>

#include "base/ods_defs.h"
#include "base/ods_objects.h"
#include "oram_controller.h"

namespace oram_impl::ods {
class OdsCache {
  const size_t max_size_;
  std::unordered_map<uint32_t, TreeNode*> cache_items_;
  std::deque<uint32_t> lru_table_;
  std::shared_ptr<OramController> oram_controller_;

 public:
  OdsCache(size_t max_size,
           const std::shared_ptr<OramController>& oram_controller)
      : max_size_(max_size), oram_controller_(oram_controller) {}

  TreeNode* Get(void);
  TreeNode* Get(uint32_t id);
  TreeNode* Put(uint32_t id, TreeNode* const item);
  uint32_t FindPosById(uint32_t id);
  uint32_t UpdatePos(uint32_t root_id);
  void Clear(void);
  void Pop(void);
  bool IsEmpty(void) { return cache_items_.empty(); }
};
}  // namespace oram_impl::ods

#endif  // ORAM_IMPL_CORE_ODS_CACHE_H_