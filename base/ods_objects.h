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
#ifndef ORAM_IMPL_BASE_ODS_OBJECTS_H_
#define ORAM_IMPL_BASE_ODS_OBJECTS_H_

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "ods_defs.h"

namespace oram_impl::ods {
struct ChildrenPos {
  uint32_t id_;
  uint32_t pos_tag_;

  ChildrenPos(): id_(invalid_mask), pos_tag_(invalid_mask) {}
  ChildrenPos(int id, int pos_tag) : id_(id), pos_tag_(pos_tag) {}
};

struct TreeNode {
  uint32_t id_;  // id is the logical address.
  uint32_t pos_tag_;
  uint32_t old_tag_ = invalid_mask;

  std::pair<uint32_t, std::string> kv_pair_;
  // An AVL Tree is a binary search tree.
  // So it needs to store the pointers to its children; but in an ORAM, the
  // pointer is the position tag that locate the unique path of each child.
  ChildrenPos children_pos_[2];
  uint32_t left_id_;
  uint32_t right_id_;
  uint32_t left_height_;
  uint32_t right_height_;

  TreeNode() = default;

  TreeNode(uint32_t id)
      : id_(id),
        pos_tag_(invalid_mask),
        left_id_(invalid_mask),
        right_id_(invalid_mask),
        left_height_(invalid_mask),
        right_height_(invalid_mask) {}
  TreeNode(uint32_t id, uint32_t pos_tag)
      : id_(id),
        pos_tag_(pos_tag),
        left_id_(invalid_mask),
        right_id_(invalid_mask),
        left_height_(invalid_mask),
        right_height_(invalid_mask) {}

  const char* GetType(void) const { return "AvlNode"; }
};

using ods_client_cache_t = std::vector<TreeNode*>;
}  // namespace oram_impl::ods

#endif  // ORAM_IMPL_BASE_ODS_OBJECTS_H_