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
#include "ods_objects.h"

#include "oram_utils.h"

namespace oram_impl::ods {
// A very simple function that converts all the fields of node into string and
// concatenate them by underscore.
std::string TreeNode::ToString(void) const {
  std::string ans;

  ans.append(std::to_string(id_) + "_");
  ans.append(std::to_string(pos_tag_) + "_");
  ans.append(std::to_string(old_tag_) + "_");
  ans.append(std::to_string(kv_pair_.first) + "_");
  ans.append(kv_pair_.second + "_");

  for (size_t i = 0; i < 2; i++) {
    ans.append(std::to_string(children_pos_[i].id_) + "_");
    ans.append(std::to_string(children_pos_[i].pos_tag_) + "_");
  }

  ans.append(std::to_string(left_id_) + "_");
  ans.append(std::to_string(right_id_) + "_");
  ans.append(std::to_string(left_height_) + "_");
  ans.append(std::to_string(right_height_));

  return ans;
}

TreeNode TreeNode::FromString(const std::string& str) {
  TreeNode out_node;

  const std::vector<std::string> data = oram_utils::split(str, '_');
  PANIC_IF(data.size() != deserialized_str_size,
           "Deserialized string is corrupted.");

  out_node.id_ = std::stoul(data[0]);
  out_node.pos_tag_ = std::stoul(data[1]);
  out_node.old_tag_ = std::stoul(data[2]);
  out_node.kv_pair_.first = std::stoul(data[3]);
  out_node.kv_pair_.second = (data[4]);
  out_node.children_pos_[0].id_ = std::stoul(data[5]);
  out_node.children_pos_[0].pos_tag_ = std::stoul(data[6]);
  out_node.children_pos_[1].id_ = std::stoul(data[7]);
  out_node.children_pos_[1].pos_tag_ = std::stoul(data[8]);
  out_node.left_id_ = std::stoul(data[9]);
  out_node.right_id_ = std::stoul(data[10]);
  out_node.left_height_ = std::stoul(data[11]);
  out_node.right_height_ = std::stoul(data[12]);

  return out_node;
}
}  // namespace oram_impl::ods