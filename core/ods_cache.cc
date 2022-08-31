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
#include "ods_cache.h"

using namespace oram_impl;
using namespace ods;

void OdsCache::Clear(void) {
  lru_table_.clear();
  cache_items_.clear();
}

uint32_t OdsCache::FindPosById(uint32_t id) {
  for (auto iter = cache_items_.begin(); iter != cache_items_.end(); iter++) {
    TreeNode* const node = iter->second;
    
    if (node->id_ == id) {
      return node->old_tag_;
    }
  }

  return 0XFFFFFFFF;
}

TreeNode* OdsCache::Get(uint32_t id) {
  auto iter = cache_items_.find(id);

  if (iter == cache_items_.end()) {
    return nullptr;
  } else {
    auto it = std::find_if(lru_table_.begin(), lru_table_.end(),
                           [id](uint32_t item_id) { return item_id == id; });

    uint32_t item_id = *it;
    lru_table_.erase(it);
    lru_table_.push_front(item_id);

    return iter->second;
  }
}

TreeNode* OdsCache::Put(uint32_t id, TreeNode* const item) {
  auto iter = cache_items_.find(id);
  auto it = std::find_if(lru_table_.begin(), lru_table_.end(),
                         [id](uint32_t item_id) { return item_id == id; });

  if (iter != cache_items_.end() && it != lru_table_.end()) {
    cache_items_.erase(iter);
    lru_table_.erase(it);
  }

  lru_table_.push_front(id);
  cache_items_[id] = item;
  TreeNode* ret = nullptr;

  // evict the item
  if (cache_items_.size() > max_size_) {
    int back = lru_table_.back();
    lru_table_.pop_back();
    iter = cache_items_.find(back);

    ret = iter->second;
    cache_items_.erase(iter);
  }

  return ret;
}

uint32_t OdsCache::UpdatePos(uint32_t root_id) {
  std::map<uint32_t, uint32_t> position_tag;

  for (auto iter = cache_items_.begin(); iter != cache_items_.end(); iter++) {
    TreeNode* const node = iter->second;
    // Generate a random position tag.
    position_tag[node->id_] = oram_controller_->RandomPosition();

    if (node->old_tag_ == invalid_mask) {
      node->old_tag_ = position_tag[node->id_];
    }

    node->pos_tag_ = position_tag[node->id_];
  }

  for (auto iter = cache_items_.begin(); iter != cache_items_.end(); iter++) {
    TreeNode* const node = iter->second;
    // Reallocate the position tag for each child node.
    node->children_pos_[0].pos_tag_ = position_tag[node->left_id_];
    node->children_pos_[1].pos_tag_ = position_tag[node->right_id_];
  }

  return position_tag[root_id];
}

void OdsCache::Pop(void) {
  const uint32_t front = lru_table_.front();
  // Find the element in the item table.
  auto iter = cache_items_.find(front);

  // Remove the element from both the LRU table and the item table.
  lru_table_.pop_front();
  cache_items_.erase(iter);
}

TreeNode* OdsCache::Get(void) {
  const uint32_t front = lru_table_.front();
  // Find the element in the item table.
  auto iter = cache_items_.find(front);
  return iter->second;
}